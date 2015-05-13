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

#include "WSPageRetrieveProgramInfo.h"
#include "WSCrypt.h"
#include "WSException.h"
#include "WSLogger.h"
#include "WSData.h"
#include "WSHelper.h"
#include "WSScheduler.h"
#include <stdlib.h>

WSPageRetrieveProgramInfo::WSPageRetrieveProgramInfo() {
}

void WSPageRetrieveProgramInfo::process(WSParameter& parameter) {
    string visitor_ip = parameter.getVisitorIP();
    bool showdetailederror = false;

    try {
        if (parameter.isGet()) {
            string token = parameter.getGetValue("token"); //mandatory
            string program_name = parameter.getGetValue("program"); //mandatory

            if (token == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter token is not supplied");

            if (program_name == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter program is not supplied");

            string username;
            WSCrypt::instance()->decodeToken(token, visitor_ip, &username);

            showdetailederror = true;

            int type;
            string description;
            string version;
            map<string, WSDProcessParameter*> optional;
            map<string, WSDProcessParameter*> nonoptional;
            list<WSDAuthor*> authors;

            WSData::instance()->getProgramInfoByName(program_name, &type, &description, &version, optional, nonoptional, authors);

            cppcms::json::value response_json;
            response_json["name"] = program_name;

            if (type == (int) HPCProcessSequential)
                response_json["type"] = "Sequential (Native)";
            else if (type == (int) HPCProcessMPI)
                response_json["type"] = "MPI Program";
            else
                response_json["type"] = "Unknown";

            response_json["description"] = description;
            response_json["version"] = version;

            typedef std::map<std::string, WSDProcessParameter*>::iterator it_type;

            auto it_type iterator = nonoptional.begin();
            int i = 0;
            while (iterator != nonoptional.end()) {
                string name = iterator->first;
                WSDProcessParameter* val = iterator->second;

                if (val->parameter_type != (int) WSDParameterPath)
                    response_json["parameters"]["mandatory"][name]["type"] = "Resource Path (String)";
                else if (val->parameter_type != (int) WSDParameterString)
                    response_json["parameters"]["mandatory"][name]["type"] = "String";
                else if (val->parameter_type != (int) WSDParameterInteger)
                    response_json["parameters"]["mandatory"][name]["type"] = "Integer Number";
                else if (val->parameter_type != (int) WSDParameterFloat)
                    response_json["parameters"]["mandatory"][name]["type"] = "Floating Point Number";
                else
                    response_json["parameters"]["mandatory"][name]["type"] = "Unknown";
                
                if (val->parameter_type == WSDParameterInteger || val->parameter_type == WSDParameterFloat) {
                    char tempbuffer [30];
                    
                    sprintf(tempbuffer, "%d", val->min);
                    string min = string(tempbuffer);
                    response_json["parameters"]["mandatory"][name]["min"] = min;
                    
                    sprintf(tempbuffer, "%d", val->max);
                    string max = string(tempbuffer);
                    response_json["parameters"]["mandatory"][name]["max"] = max;
                }
                
                response_json["parameters"]["mandatory"][name]["default"] = val->default_value;
                
                response_json["parameters"]["mandatory"][name]["description"] = val->description;
                
                delete val;
                
                iterator++;
                i++;
            }            
            
            iterator = optional.begin();
            i = 0;
            while (iterator != optional.end()) {
                string name = iterator->first;
                WSDProcessParameter* val = iterator->second;

                if (val->parameter_type != (int) WSDParameterPath)
                    response_json["parameters"]["optional"][name]["type"] = "Resource Path (String)";
                else if (val->parameter_type != (int) WSDParameterString)
                    response_json["parameters"]["optional"][name]["type"] = "String";
                else if (val->parameter_type != (int) WSDParameterInteger)
                    response_json["parameters"]["optional"][name]["type"] = "Integer Number";
                else if (val->parameter_type != (int) WSDParameterFloat)
                    response_json["parameters"]["optional"][name]["type"] = "Floating Point Number";
                else
                    response_json["parameters"]["optional"][name]["type"] = "Unknown";
                
                if (val->parameter_type == WSDParameterInteger || val->parameter_type == WSDParameterFloat) {
                    char tempbuffer [30];
                    
                    sprintf(tempbuffer, "%d", val->min);
                    string min = string(tempbuffer);
                    response_json["parameters"]["optional"][name]["min"] = min;
                    
                    sprintf(tempbuffer, "%d", val->max);
                    string max = string(tempbuffer);
                    response_json["parameters"]["optional"][name]["max"] = max;
                }
                
                response_json["parameters"]["optional"][name]["default"] = val->default_value;
                
                response_json["parameters"]["optional"][name]["description"] = val->description;
                
                delete val;
                
                iterator++;
                i++;
            }
            
            if (type == (int) HPCProcessMPI) {
                response_json["parameters"]["optional"]["nodes"]["type"] = "Integer Number";
                response_json["parameters"]["optional"]["nodes"]["default"] = "1";
                response_json["parameters"]["optional"]["nodes"]["description"] = "Number of nodes";
                response_json["parameters"]["optional"]["ppn"]["type"] = "Integer Number";
                response_json["parameters"]["optional"]["ppn"]["default"] = "1";
                response_json["parameters"]["optional"]["ppn"]["description"] = "Process per nodes";
            }
            
            typedef std::list<WSDAuthor*>::iterator it_type2;

            auto it_type2 iterator2 = authors.begin();
            i = 1;
            while (iterator2 != authors.end()) {
                char tempbuffer [30];
                
                WSDAuthor* author = *iterator2;
                sprintf(tempbuffer, "%d.Author", i);
                string key = string(tempbuffer);
                response_json["authors"][key]["name"] = author->name;
                response_json["authors"][key]["surname"] = author->surname;
                response_json["authors"][key]["email"] = author->email;
                response_json["authors"][key]["phone"] = author->phone;
                response_json["authors"][key]["affiliation"] = author->affiliation;
                
                iterator2++;
                i++;
                
                delete author;
            }

            response_json["status"] = "success";

            parameter.sendResponse(response_json);
        } else {
            throw WSException(WS_EXCEPTION_BAD_HTTP_METHOD, "The method can be accessed via HTTP GET");
        }
    } catch (WSException& e) {
        cppcms::json::value response_json;
        if (showdetailederror)
            response_json["description"] = e.exceptionString();
        else
            response_json["description"] = "The request had bad syntax or was impossible to be satisified.";
        response_json["status"] = "error";

        parameter.sendResponse(response_json);

        WSLogger::instance()->log("WSPageRetrieveProgramInfo", visitor_ip, e);
    }
}

