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

#include "WSPageDeleteJob.h"
#include "WSCrypt.h"
#include "WSException.h"
#include "WSLogger.h"
#include "WSData.h"
#include "WSHelper.h"
#include "WSScheduler.h"
#include <stdlib.h>

WSPageDeleteJob::WSPageDeleteJob() {
}

void WSPageDeleteJob::process(WSParameter& parameter) {
    string visitor_ip = parameter.getVisitorIP();
    bool showdetailederror = false;

    try {
        if (parameter.isDelete()) {
            string token = parameter.getGetValue("token"); //mandatory
            string jobidinstr = parameter.getGetValue("jobid"); //mandatory
            
            if (token == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter token is not supplied");
            
            if (jobidinstr == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter jobid is not supplied");
            
            string username;
            bool isExpired;
            int remainingExpireTime;
            WSCrypt::instance()->decodeToken(token, visitor_ip, &username, &isExpired, &remainingExpireTime);
            
            showdetailederror = true;
            
            int jobid = atoi(jobidinstr.c_str());
            
            WSDJob jobdata;
            WSData::instance()->getJobByID(false, jobid, jobdata);
                      
            WSHPCWork* workitem = new WSHPCWork();
            workitem->jobtype = HPCJobDelete;
            workitem->clientip = visitor_ip;
            workitem->jobid = jobid;
            workitem->hpcjobid = jobdata.hpc_job_id;
            workitem->packageid = jobdata.package_id;
            workitem->hpcid = jobdata.hpc_id;
            workitem->callbackurl = jobdata.callbackurl;
            
            WSScheduler::instance()->addHPCWork(workitem);
                      
            cppcms::json::value response_json;
            response_json["jobid"] = jobidinstr;
            response_json["status"] = "success";

            parameter.sendResponse(response_json);
        } else {
            throw WSException(WS_EXCEPTION_BAD_HTTP_METHOD, "The method can be accessed via HTTP DELETE");
        }
    } catch (WSException& e) {
        cppcms::json::value response_json;
        if (showdetailederror)
            response_json["description"] = e.exceptionString();
        else
            response_json["description"] = "The request had bad syntax or was impossible to be satisified.";
        response_json["status"] = "error";

        parameter.sendResponse(response_json);

        WSLogger::instance()->log("WSPageDeleteJob", visitor_ip, e);
    }
}

