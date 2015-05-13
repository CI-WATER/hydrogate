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

#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h> 
#include <cppcms/url_mapper.h> 
#include <cppcms/http_file.h>
#include <cppcms/cache_interface.h>
#include <iostream>
#include <map>
#include <unistd.h>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include "WSLogger.h"
#include "WSPageRequestToken.h"
#include "WSPageUploadPackage.h"
#include "WSPageRetrievePackageStatus.h"
#include "WSPageSubmitJob.h"
#include "WSPageRetrieveJobStatus.h"
#include "WSPageReturnHPCNames.h"
#include "WSPageReturnHPCProgramNames.h"
#include "WSPageRetrieveTokenExpireTime.h"
#include "WSPageRetrieveProgramInfo.h"
#include "WSPageDeletePackage.h"
#include "WSPageDeleteJob.h"
#include "WSParameter.h"
#include "WSScheduler.h"
#include "WSCrypt.h"

using namespace std;

class WSProxy : public cppcms::application {
private:
    WSPageRequestToken* pageRequestToken_;
    WSPageUploadPackage* pageUploadPackage_;
    WSPageRetrievePackageStatus* pageRetrievePackageStatus_;
    WSPageSubmitJob* pageSubmitJob_;
    WSPageRetrieveJobStatus* pageRetrieveJobStatus_;
    WSPageReturnHPCNames* pageReturnHPCNames_;
    WSPageReturnHPCProgramNames* pageReturnHPCProgramNames_;
    WSPageRetrieveTokenExpireTime* pageRetrieveTokenExpireTime_;
    WSPageRetrieveProgramInfo* pageRetrieveProgramInfo_;
    WSPageDeletePackage* pageDeletePackage_;
    WSPageDeleteJob* pageDeleteJob_;
    
    bool isInitialized_;
    
public:

    WSProxy(cppcms::service &srv) :
    cppcms::application(srv) {
        pageRequestToken_ = new WSPageRequestToken();
        pageUploadPackage_ = new WSPageUploadPackage();
        pageRetrievePackageStatus_ = new WSPageRetrievePackageStatus();
        pageSubmitJob_ = new WSPageSubmitJob();
        pageRetrieveJobStatus_ = new WSPageRetrieveJobStatus();
        pageReturnHPCNames_ = new WSPageReturnHPCNames();
        pageReturnHPCProgramNames_ = new WSPageReturnHPCProgramNames();
        pageRetrieveTokenExpireTime_ = new WSPageRetrieveTokenExpireTime();
        pageRetrieveProgramInfo_ = new WSPageRetrieveProgramInfo();
        pageDeletePackage_ = new WSPageDeletePackage();
        pageDeleteJob_ = new WSPageDeleteJob();
        
        isInitialized_ = false;
        
        dispatcher().assign("/request_token/", &WSProxy::requestToken, this);
        mapper().assign("request_token", "/request_token/");
        
        dispatcher().assign("/upload_package/", &WSProxy::uploadPackage, this);
        mapper().assign("upload_package", "/upload_package/");
        
        dispatcher().assign("/submit_job/", &WSProxy::submitJob, this);
        mapper().assign("submit_job", "/submit_job/");
        
        dispatcher().assign("/retrieve_package_status", &WSProxy::retrievePackageStatus, this);
        mapper().assign("retrieve_package_status", "/retrieve_package_status");
        
        dispatcher().assign("/retrieve_job_status", &WSProxy::retrieveJobStatus, this);
        mapper().assign("retrieve_job_status", "/retrieve_job_status");        
        
        dispatcher().assign("/return_hpc_names", &WSProxy::returnHPCNames, this);
        mapper().assign("return_hpc_names", "/return_hpc_names");
        
        dispatcher().assign("/return_hpc_program_names/", &WSProxy::returnHPCProgramNames, this);
        mapper().assign("return_hpc_program_names", "/return_hpc_program_names/");
        
        dispatcher().assign("/retrieve_token_expire_time", &WSProxy::retrieveTokenExpireTime, this);
        mapper().assign("retrieve_token_expire_time", "/retrieve_token_expire_time");
    
        dispatcher().assign("/retrieve_program_info", &WSProxy::retrieveProgramInfo, this);
        mapper().assign("retrieve_program_info", "/retrieve_program_info");
        
        dispatcher().assign("/delete_package", &WSProxy::deletePackage, this);
        mapper().assign("delete_package", "/delete_package");
        
        dispatcher().assign("/delete_job", &WSProxy::deleteJob, this);
        mapper().assign("delete_job", "/delete_job");
        
        dispatcher().assign("/", &WSProxy::mainEntry, this);
        mapper().assign("/");
    }
    
    void checkIsInitialized() {
        if (isInitialized_)
            return;
        
        isInitialized_ = true;
        
        WSData::instance()->setConnectionString(settings().get<string>("database.connectionstring"));
        WSCrypt::instance()->setTokenKey(settings().get<string>("security.aeskey"));    
        WSCrypt::instance()->setTokenSalt1(settings().get<int>("security.usersalt1"));
        WSCrypt::instance()->setTokenSalt2(settings().get<int>("security.usersalt2"));
        WSCrypt::instance()->setTokenExpiretionDurationInSec(settings().get<int>("security.tokenexpiredurationinseconds"));
        WSScheduler::instance()->setKeydata(settings().get<string>("security.aeskey"));
        WSScheduler::instance()->setInitialThreadCount(settings().get<int>("scheduler.workeritemcount"));
        WSScheduler::instance()->setMaxConnectionPerHPC(settings().get<int>("scheduler.maxsshconnectionperhpc"));
        WSScheduler::instance()->setSCPBufsiz(settings().get<int>("scheduler.scpbufsiz"));
        WSScheduler::instance()->setStdoutBufsiz(settings().get<int>("scheduler.stdoutbufsiz"));
        WSScheduler::instance()->setJobStatusCheckDelayInSeconds(settings().get<int>("scheduler.jobstatuscheckdelayinseconds"));
        WSScheduler::instance()->setOutputFolder(settings().get<string>("scheduler.outputfolder"));
        WSScheduler::instance()->setInputLocalMode(settings().get<bool>("scheduler.isinputinlocalmode"));
        WSScheduler::instance()->setOutputLocalMode(settings().get<bool>("scheduler.isoutputinlocalmode"));
        WSScheduler::instance()->setInputURL(settings().get<string>("scheduler.inputurl"));
        WSScheduler::instance()->setOutputURL(settings().get<string>("scheduler.outputurl"));
        WSScheduler::instance()->setOutputURLSrc(settings().get<string>("scheduler.outputurlsrc"));
        WSScheduler::instance()->setIsAbsoluteURL(settings().get<bool>("scheduler.isabsoluteurl"));
        
        WSScheduler::instance()->initialize();
    }
    
    void deleteJob() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageDeleteJob_->process(parameter);
    }
    
    void deletePackage() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageDeletePackage_->process(parameter);
    }
    
    void retrieveProgramInfo() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageRetrieveProgramInfo_->process(parameter);
    }
    
    void retrieveTokenExpireTime() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageRetrieveTokenExpireTime_->process(parameter);
    }
    
    void returnHPCProgramNames() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageReturnHPCProgramNames_->process(parameter);
    }
    
    void returnHPCNames() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageReturnHPCNames_->process(parameter);
    }
    
    void requestToken() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageRequestToken_->process(parameter);
    }
    
    void uploadPackage() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageUploadPackage_->process(parameter);
    }
    
    void retrievePackageStatus() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageRetrievePackageStatus_->process(parameter);
    }
    
    void submitJob() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageSubmitJob_->process(parameter);
    }
    
    void retrieveJobStatus() {
        checkIsInitialized();
        WSParameter parameter(&request(), &response(), &cache(), &settings());
        pageRetrieveJobStatus_->process(parameter);
    }
    
    void mainEntry() {
    }
};

int main(int argc, char ** argv) {
    
    ssh_threads_set_callbacks(ssh_threads_get_pthread());
    ssh_init();
    
    WSLogger::instance()->log("Starting HydroGate...");
    
    try {
        cppcms::service srv(argc, argv);
        srv.applications_pool().mount(
                cppcms::applications_factory<WSProxy>()
                );
        srv.run();
    } catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
    }
}
