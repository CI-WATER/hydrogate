/*
    HydroGate is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Regarding this entire document or any portion of it , the author 
    makes no guarantees and is not responsible for any damage resulting 
    from its use.

    Ahmet Artu Yildirim
    Utah State University
    ahmetartu@aggiemail.usu.edu
*/

#include "WSPageUploadPackage.h"
#include "WSCrypt.h"
#include "WSException.h"
#include "WSLogger.h"
#include "WSData.h"
#include "WSHelper.h"
#include "WSScheduler.h"
#include <curl/curl.h>

WSPageUploadPackage::WSPageUploadPackage() {
}

void WSPageUploadPackage::process(WSParameter& parameter) {
    string visitor_ip = parameter.getVisitorIP();
    bool showdetailederror = false;

    try {
        if (parameter.isPost()) {
            string token = parameter.getPostValue("token"); //mandatory
            string packagepath = parameter.getPostValue("package"); //mandatory
            string hpcname = parameter.getPostValue("hpc"); //mandatory for now

            if (token == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter token is not supplied");

            if (packagepath == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter package is not supplied");

            if (hpcname == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter hpc is not supplied");

            string callbackurl = "";

            string username;
            WSCrypt::instance()->decodeToken(token, visitor_ip, &username);

            WSDUser user_rec;
            WSData::instance()->getUserByName(username, &user_rec);

            int hpc_id = 0;
            bool hasRightOnHPC = WSData::instance()->hasUserRightOnHPC(user_rec.user_id, hpcname, &hpc_id);

            if (!hasRightOnHPC) {
                throw WSException(WS_EXCEPTION_NOT_FOUND, "No access permission on HPC " + hpcname + " for user " + username);
            }

            showdetailederror = true;

            string inputfolder = parameter.getSetting<string>("scheduler.inputfolder");
            if (inputfolder[inputfolder.length() - 1] != '/') {
                    inputfolder = inputfolder + "/";
            }
            
            bool islocalmode = parameter.getSetting<bool>("scheduler.isinputinlocalmode");
            if (islocalmode) {
                string datalocation = inputfolder;

                if (packagepath[0] == '/') {
                    packagepath.erase(0, 1);
                }

                // get safer path, do not allow parent folder indirection
                WSHelper::instance()->replaceStringInPlace(packagepath, "..", ".", true);
                packagepath = datalocation + packagepath;

                bool iszipfile = WSHelper::instance()->checkFileExtension(packagepath, ".zip");

                if (!iszipfile) {
                    throw WSException(WS_EXCEPTION_IO, "The file [" + packagepath + "] is not a zip file");
                }

                bool isexist = WSHelper::instance()->fileExists(packagepath);

                if (!isexist) {
                    throw WSException(WS_EXCEPTION_IO, "The file [" + packagepath + "] does not exist, nor readable");
                }
            } else {
                // if the package is at iRODS, starting with '\' character, do not consider isabsoluteurl
                if (packagepath[0] != '/') {
                    bool isabsoluteurl = parameter.getSetting<bool>("scheduler.isabsoluteurl");

                    if (!isabsoluteurl) {
                        string remotefilepath = parameter.getSetting<string>("scheduler.inputurl");
                        WSHelper::instance()->replaceStringInPlace(remotefilepath, "$file", packagepath, true);
                        packagepath = remotefilepath;
                    }
                }

                bool iszipfile = WSHelper::instance()->checkFileExtension(packagepath, ".zip");

                if (!iszipfile) {
                    throw WSException(WS_EXCEPTION_IO, "The file [" + packagepath + "] is not a zip file");
                }

                /* 
                 * HEAD request is not supported by grizzly and some others, and also slows down the function
                CURL *curl;
                CURLcode res;

                curl_global_init(CURL_GLOBAL_DEFAULT);
                curl = curl_easy_init();

                curl_easy_setopt(curl, CURLOPT_URL, packagepath.c_str());
                curl_easy_setopt(curl, CURLOPT_NOBODY, true);
                curl_easy_perform(curl);
                long responsecode;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responsecode);

                if (responsecode != 200) {
                    throw WSException(WS_EXCEPTION_IO, "The file at [" + packagepath + "] does not exist, nor readable.");
                }
                 */
            }

            bool packagecallbackenabled = parameter.getSetting<bool>("callback.packagecallbackenabled");

            if (packagecallbackenabled) {
                bool isabsolutecallbackurl = parameter.getSetting<bool>("callback.isabsolutecallbackurl");

                string paramcallback = parameter.getPostValue("callback");
                if (isabsolutecallbackurl && paramcallback != "") {
                    callbackurl = paramcallback;
                } else if (!isabsolutecallbackurl) {
                    callbackurl = parameter.getSetting<string>("callback.packagecallbackurl");
                }
            }

            int packageid = WSData::instance()->createPackage(false, user_rec.user_id, hpc_id, callbackurl);

            WSHPCWork* workitem = new WSHPCWork();
            workitem->jobtype = HPCJobPackageUpload;
            workitem->clientip = visitor_ip;
            workitem->hpcid = hpc_id;
            workitem->jobid = -1;
            workitem->packageid = packageid;
            workitem->packagepath = packagepath;
            workitem->userid = user_rec.user_id;
            workitem->inputfolder = inputfolder;
            workitem->callbackurl = callbackurl;

            WSScheduler::instance()->addHPCWork(workitem);

            char packageidbuffer [15];
            sprintf(packageidbuffer, "%d", packageid);

            cppcms::json::value response_json;
            response_json["packageid"] = packageidbuffer;
            response_json["status"] = "success";

            parameter.sendResponse(response_json);
        } else {
            throw WSException(WS_EXCEPTION_BAD_HTTP_METHOD, "The method can be accessed via HTTP POST");
        }
    } catch (WSException& e) {
        cppcms::json::value response_json;
        if (showdetailederror)
            response_json["description"] = e.exceptionString();
        else
            response_json["description"] = "The request had bad syntax or was impossible to be satisified.";
        response_json["status"] = "error";

        parameter.sendResponse(response_json);

        WSLogger::instance()->log("WSPageUploadPackage", visitor_ip, e);
    }
}

