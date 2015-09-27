/* 
 * File:   WSPageSubmitJob.cpp
 * Author: artu
 * 
 * Created on February 11, 2014, 3:23 PM
 */

#include "WSPageSubmitJob.h"
#include "WSCrypt.h"
#include "WSException.h"
#include "WSLogger.h"
#include "WSHelper.h"
#include <stdlib.h>

WSPageSubmitJob::WSPageSubmitJob() {
}

WSPageSubmitJob::~WSPageSubmitJob() {
}

void WSPageSubmitJob::process(WSParameter& parameter) {
    string visitor_ip = parameter.getVisitorIP();
    bool showdetailederror = false;

    try {
        if (parameter.isPost()) {
            string token = parameter.getPostValue("token"); //mandatory
            string strpackageid = parameter.getPostValue("packageid"); //mandatory
            string jobdefinition = parameter.getPostValue("jobdefinition"); // mandatory

            if (token == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter token is not supplied");

            if (strpackageid == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter packageid is not supplied");

            if (jobdefinition == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter jobdefinition is not supplied");

            string callbackurl = "";

            //NOTE: isabsolutecallbackurl is not implemented yet for output package (pass the value in HPCJobCheckResult), to be determined later.
            //string outputpackageurl = parameter.getPostValue("outputpackageurl"); //optional

            long int packageid = strtol(strpackageid.c_str(), NULL, 10);
            if (packageid == 0L)
                throw WSException(WS_EXCEPTION_BAD_PARAM, string("Parameter packageid cannot be casted to integer. Input: ") + strpackageid);

            string username;
            WSCrypt::instance()->decodeToken(token, visitor_ip, &username);

            WSDUser user_rec;
            WSData::instance()->getUserByName(username, &user_rec);

            showdetailederror = true;

            int jobid;
            int hpcid;
            WSDHPCWorkState state;
            string folder = "";
            string expectedoutputpath = "";

            WSData::instance()->getPackageFolderInformationByID(false, packageid, &state, &hpcid, &folder);

            if (state == WSDPackageDeleted)
                throw WSException(WS_EXCEPTION_FATAL, string("Package is deleted."));

            if (state != WSDPackageStateDone)
                throw WSException(WS_EXCEPTION_FATAL, string("Package upload is not finished."));

            std::stringstream jobdefinitionstr;
            jobdefinitionstr << jobdefinition;

            cppcms::json::value json_jd;
            bool isloadedok = json_jd.load(jobdefinitionstr, true);

            if (!isloadedok)
                throw WSException(WS_EXCEPTION_JSON_PARSE, "Failed to parse jbo definition");

            string program_name;
            string walltime;
            string outputlist = "";

            try {
                program_name = json_jd.get<string>("program");
            } catch (...) {
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Failed to get program value in job definition");
            }

            try {
                walltime = json_jd.get<string>("walltime");
            } catch (...) {
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Failed to get walltime value in job definition");
            }

            try {
                vector<string> outputlistarray = json_jd.get < std::vector < std::string > > ("outputlist");
                for (vector<int>::size_type i = 0; i != outputlistarray.size(); i++) {
                    outputlist += WSHelper::instance()->getSafeFileName(outputlistarray[i]) + " ";
                }
            } catch (...) {
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Failed to get outputlist value in job definition");
            }

            vector<string> workflowitems;
            bool isworkflow = false;

            try {
                workflowitems = json_jd.get < std::vector < std::string > > ("workflow");
                isworkflow = true;
            } catch (...) {
                isworkflow = false;
            }

            if (isworkflow) {
                vector<WSWorkflowTask*>* workflowTasks = new vector<WSWorkflowTask*>();

                int maxnodes = 1;
                int maxppn = 1;

                for (vector<int>::size_type i = 0; i != workflowitems.size(); i++) {
                    string workflowitem = workflowitems[i];
                    string program_json_name = workflowitem + ".program";
                    string workflow_program_name;

                    try {
                        workflow_program_name = json_jd.get<string>(program_json_name);
                    } catch (...) {
                        WSScheduler::instance()->freeWorkflowArray(workflowTasks);
                        throw WSException(WS_EXCEPTION_BAD_PARAM, "Failed to parse workflow item " + workflowitem);
                    }

                    WSWorkflowTask* workflowTask = new WSWorkflowTask();
                    workflowTasks->push_back(workflowTask);

                    workflowTask->program_name = workflow_program_name;

                    int program_id;
                    int program_type;
                    string install_path;

                    bool is_program_installed = WSData::instance()->isProgramInstalledOnHPC(hpcid, workflow_program_name, &program_id, &program_type, &install_path);

                    workflowTask->program_type = (HPCProgramType) program_type;
                    workflowTask->program_path = install_path + workflow_program_name;

                    if (!is_program_installed) {
                        WSScheduler::instance()->freeWorkflowArray(workflowTasks);
                        throw WSException(WS_EXCEPTION_NOT_FOUND, "Program " + workflow_program_name + " is not installed on the system");
                    }

                    map<string, WSDProcessParameter*> systemcontrolled;
                    map<string, WSDProcessParameter*> optional;
                    map<string, WSDProcessParameter*> nonoptional;

                    WSData::instance()->getProgramParameters(program_id, systemcontrolled, optional, nonoptional);

                    typedef std::map<std::string, WSDProcessParameter*>::iterator it_type;

                    string process_parameters = "";

                    // first check optional parameters
                    auto it_type iterator = optional.begin();
                    while (iterator != optional.end()) {
                        string name = iterator->first;
                        WSDProcessParameter* val = iterator->second;

                        string jsonname = workflowitem + ".parameters." + name;
                        string param;

                        try {
                            param = json_jd.get<string>(jsonname);
                        } catch (...) {
			    if (val->default_value != "")
			    	process_parameters += val->option + " " + val->default_value + " ";
                            iterator++;
                            continue;
                        }

                        if (val->parameter_type == WSDParameterPath) {
                            param = WSHelper::instance()->getSafeFileName(param);
                        }

                        process_parameters += val->option + " " + param + " ";

                        iterator++;
                    }

                    // second check nonoptional parameters, if not provided throw exception
                    iterator = nonoptional.begin();
                    while (iterator != nonoptional.end()) {
                        string name = iterator->first;
                        WSDProcessParameter* val = iterator->second;

                        string jsonname = workflowitem + ".parameters." + name;
                        string param;

                        try {
                            param = json_jd.get<string>(jsonname);
                        } catch (...) {
                            WSData::instance()->freeProcessParameterMap(systemcontrolled);
                            WSData::instance()->freeProcessParameterMap(optional);
                            WSData::instance()->freeProcessParameterMap(nonoptional);

                            WSScheduler::instance()->freeWorkflowArray(workflowTasks);

                            throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter " + name + " is not provided in workflow item " + workflowitem);
                        }

                        if (val->parameter_type == WSDParameterPath) {
                            param = WSHelper::instance()->getSafeFileName(param);
                        }

                        process_parameters += val->option + " " + param + " ";

                        iterator++;
                    }

                    // last add systemcontrolled parameters
		    /*
                    iterator = systemcontrolled.begin();
                    while (iterator != systemcontrolled.end()) {
                        string name = iterator->first;
                        WSDProcessParameter* val = iterator->second;

                        process_parameters += val->option + " " + val->default_value + " ";

                        iterator++;
                    } */

                    WSData::instance()->freeProcessParameterMap(systemcontrolled);
                    WSData::instance()->freeProcessParameterMap(optional);
                    WSData::instance()->freeProcessParameterMap(nonoptional);

                    workflowTask->processparameters = process_parameters;
                    workflowTask->nodes = 1;
                    workflowTask->ppn = 1;

                    if (workflowTask->program_type == HPCProcessMPI) {
                        string par;

                        try {
                            par = json_jd.get<string>(workflowitem + ".parameters.nodes");
                            workflowTask->nodes = atoi(par.c_str());

                            if (workflowTask->nodes > maxnodes) {
                                maxnodes = workflowTask->nodes;
                            }
                        } catch (...) {
                        }

                        try {
                            par = json_jd.get<string>(workflowitem + ".parameters.ppn");
                            workflowTask->ppn = atoi(par.c_str());

                            if (workflowTask->ppn > maxppn) {
                                maxppn = workflowTask->ppn;
                            }
                        } catch (...) {
                        }
                    }
                }

                string description = " ";

                try {
                    description = json_jd.get<string>("description");
                } catch (...) {
                }

                bool jobcallbackenabled = parameter.getSetting<bool>("callback.jobcallbackenabled");

                if (jobcallbackenabled) {
                    bool isabsolutecallbackurl = parameter.getSetting<bool>("callback.isabsolutecallbackurl");

                    string paramcallback = parameter.getPostValue("callback");
                    if (isabsolutecallbackurl && paramcallback != "") {
                        callbackurl = paramcallback;
                    } else if (!isabsolutecallbackurl) {
                        callbackurl = parameter.getSetting<string>("callback.jobcallbackurl");
                    }
                }

                jobid = WSData::instance()->createJob(false, user_rec.user_id, hpcid, packageid, description, jobdefinition, callbackurl);

                WSHPCWork* workitem = new WSHPCWork();
                workitem->isworkflow = true;
                workitem->jobtype = HPCJobSubmit;
                workitem->clientip = visitor_ip;
                workitem->jobid = jobid;
                workitem->hpcid = hpcid;
                workitem->packageid = packageid;
                workitem->userid = user_rec.user_id;
                workitem->packagefoldername = folder;
                workitem->workflow_tasks = workflowTasks;
                workitem->walltime = walltime;
                workitem->outputlist = outputlist;
                workitem->nodes = maxnodes;
                workitem->ppn = maxppn;
                workitem->callbackurl = callbackurl;
                //workitem->outputpackageurl = outputpackageurl;

                WSScheduler::instance()->addHPCWork(workitem);
            } else {
                int program_id;
                int program_type;
                string install_path;

                bool is_program_installed = WSData::instance()->isProgramInstalledOnHPC(hpcid, program_name, &program_id, &program_type, &install_path);

                if (!is_program_installed)
                    throw WSException(WS_EXCEPTION_NOT_FOUND, "Program " + program_name + " is not installed on the system");

                map<string, WSDProcessParameter*> systemcontrolled;
                map<string, WSDProcessParameter*> optional;
                map<string, WSDProcessParameter*> nonoptional;

                WSData::instance()->getProgramParameters(program_id, systemcontrolled, optional, nonoptional);

                typedef std::map<std::string, WSDProcessParameter*>::iterator it_type;

                string process_parameters = "";

                // first check optional parameters
                auto it_type iterator = optional.begin();
                while (iterator != optional.end()) {
                    string name = iterator->first;
                    WSDProcessParameter* val = iterator->second;

                    string jsonname = "parameters." + name;
                    string param;

                    try {
                        param = json_jd.get<string>(jsonname);
                    } catch (...) {
			if (val->default_value != "")
                        	process_parameters += val->option + " " + val->default_value + " ";
                        iterator++;
                        continue;
                    }

                    if (val->parameter_type == WSDParameterPath) {
                        param = WSHelper::instance()->getSafeFileName(param);
                    }

                    process_parameters += val->option + " " + param + " ";

                    iterator++;
                }

                // second check nonoptional parameters, if not provided throw exception
                iterator = nonoptional.begin();
                while (iterator != nonoptional.end()) {
                    string name = iterator->first;
                    WSDProcessParameter* val = iterator->second;

                    string jsonname = "parameters." + name;
                    string param;

                    try {
                        param = json_jd.get<string>(jsonname);
                    } catch (...) {
                        WSData::instance()->freeProcessParameterMap(systemcontrolled);
                        WSData::instance()->freeProcessParameterMap(optional);
                        WSData::instance()->freeProcessParameterMap(nonoptional);

                        throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter " + name + " is not provided");
                    }

                    if (val->parameter_type == WSDParameterPath) {
                        param = WSHelper::instance()->getSafeFileName(param);
                    }

                    process_parameters += val->option + " " + param + " ";

                    iterator++;
                }

                // last add systemcontrolled parameters
		/*
                iterator = systemcontrolled.begin();
                while (iterator != systemcontrolled.end()) {
                    string name = iterator->first;
                    WSDProcessParameter* val = iterator->second;

                    process_parameters += val->option + " " + val->default_value + " ";

                    iterator++;
                } */

                WSData::instance()->freeProcessParameterMap(systemcontrolled);
                WSData::instance()->freeProcessParameterMap(optional);
                WSData::instance()->freeProcessParameterMap(nonoptional);

                string description = " ";

                try {
                    description = json_jd.get<string>("description");
                } catch (...) {
                }

                bool jobcallbackenabled = parameter.getSetting<bool>("callback.jobcallbackenabled");

                if (jobcallbackenabled) {
                    bool isabsolutecallbackurl = parameter.getSetting<bool>("callback.isabsolutecallbackurl");

                    string paramcallback = parameter.getPostValue("callback");
                    if (isabsolutecallbackurl && paramcallback != "") {
                        callbackurl = paramcallback;
                    } else if (!isabsolutecallbackurl) {
                        callbackurl = parameter.getSetting<string>("callback.jobcallbackurl");
                    }
                }

                jobid = WSData::instance()->createJob(false, user_rec.user_id, hpcid, packageid, description, jobdefinition, callbackurl);

                WSHPCWork* workitem = new WSHPCWork();
                workitem->isworkflow = false;
                workitem->jobtype = HPCJobSubmit;
                workitem->clientip = visitor_ip;
                workitem->jobid = jobid;
                workitem->hpcid = hpcid;
                workitem->packageid = packageid;
                workitem->userid = user_rec.user_id;
                workitem->packagefoldername = folder;
                workitem->processparameters = process_parameters;
                workitem->programtype = (HPCProgramType) program_type;
                workitem->nodes = 1;
                workitem->ppn = 1;
                workitem->programpath = install_path + program_name;
                workitem->walltime = walltime;
                workitem->outputlist = outputlist;
                workitem->callbackurl = callbackurl;
                //workitem->outputpackageurl = outputpackageurl;

                if (workitem->programtype == HPCProcessMPI) {
                    string par;

                    try {
                        par = json_jd.get<string>("parameters.nodes");
                        workitem->nodes = atoi(par.c_str());
                    } catch (...) {

                    }

                    try {
                        par = json_jd.get<string>("parameters.ppn");
                        workitem->ppn = atoi(par.c_str());
                    } catch (...) {

                    }
                }

                WSScheduler::instance()->addHPCWork(workitem);
            }

            bool islocalmode = parameter.getSetting<bool>("scheduler.isoutputinlocalmode");
            char tempbuff[100];
            if (islocalmode) {
                sprintf(tempbuff, "result_job%d.zip", jobid);
                string resultzipfilename = string(tempbuff);
                expectedoutputpath = WSScheduler::instance()->getOutputFolder() + resultzipfilename;
            } else {
                //bool isabsolutecallbackurl = parameter.getSetting<bool>("callback.isabsolutecallbackurl");
                //if (isabsolutecallbackurl) {
                //        expectedoutputpath = outputpackageurl;
                //} else {
                string outputurlsrc = parameter.getSetting<string>("scheduler.outputurlsrc");
                if (outputurlsrc[outputurlsrc.length() - 1] != '/') {
                    outputurlsrc = outputurlsrc + "/";
                }

                sprintf(tempbuff, "%d/result.zip", jobid);
                string resultzipfilename = string(tempbuff);
                expectedoutputpath = outputurlsrc + resultzipfilename;
                //}
            }

            sprintf(tempbuff, "%d", jobid);

            cppcms::json::value response_json;
            response_json["jobid"] = tempbuff;
            response_json["outputpath"] = expectedoutputpath;
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
            response_json["description"] = "The request had bad syntax or was impossible to be satisified";
        response_json["status"] = "error";

        parameter.sendResponse(response_json);

        WSLogger::instance()->log("WSPageSubmitJob", visitor_ip, e);
    } catch (...) {
        cppcms::json::value response_json;
        response_json["description"] = "The request had bad syntax or was impossible to be satisified";
        response_json["status"] = "error";

        parameter.sendResponse(response_json);

        WSLogger::instance()->log("WSPageSubmitJob", visitor_ip, "Unidentified exception");
    }
}
