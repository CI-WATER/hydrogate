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

#include "WSScheduler.h"
#include "WSException.h"
#include "WSCrypt.h"
#include "WSHelper.h"
#include <uuid/uuid.h>
#include <curl/curl.h>
#include <cstdio>

struct RemoteFile {
    string filename;
    CURL *curl;
    ssh_scp scp;
    bool isinitialized;
    WSSshSession* sshsession;
    string exception;
};

struct RemoteFile2 {
    ssh_scp scp;
    WSSshSession* sshsession;
    string exception;
};

WSScheduler* WSScheduler::instance_ = NULL;
int WSScheduler::thread_num_ = 0;

WSScheduler::WSScheduler() {
    hpcsessionpool_ = new map<int, WSSshSessionPool*>();
    workitempool_ = new list<WSScheduler::WSSchedulerWorkItem*>();
    hpcworkqueue_ = new list<WSHPCWork*>();
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
    pthread_mutex_init(&mutex_cpool_, NULL);
    keydata_ = "";
    initial_thread_count_ = 50;
    max_connection_per_hpc_ = 10;
    scp_bufsiz_ = 102400;
    stdout_bufsiz_ = 512;
    job_status_check_delay_in_seconds_ = 10;
    isinputinlocalmode_ = true;
    isoutputinlocalmode_ = true;
    inputurl_ = "";
    outputurl_ = "";
    outputurlsrc_ = "";
}

WSScheduler::~WSScheduler() {
    delete hpcsessionpool_;
    delete workitempool_;
    delete hpcworkqueue_;
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_cpool_);
}

WSScheduler* WSScheduler::instance() {
    if (!instance_) {
        instance_ = new WSScheduler();
    }

    return instance_;
}

void WSScheduler::initialize() {
    pthread_attr_t bckattr;
    pthread_attr_init(&bckattr);
    pthread_attr_setdetachstate(&bckattr, PTHREAD_CREATE_JOINABLE);

    int rc;
    rc = pthread_create(&background_worker_thread_, &bckattr, WSScheduler::backgroundWorkerEntry, (void *) this);
    if (rc) {
        pthread_attr_destroy(&bckattr);
        WSLogger::instance()->log("WSScheduler", "Failed to create background worker");
    }

    int i;
    for (i = 0; i < initial_thread_count_; i++) {
        WSScheduler::WSSchedulerWorkItem* witem = new WSScheduler::WSSchedulerWorkItem();
        witem->scheduler = this;
        witem->thread_num = WSScheduler::thread_num_++;
        witem->state = WITEM_WAITING;
        witem->cont = true;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        int rc;
        rc = pthread_create(&witem->job_submit_thread, &attr, WSScheduler::workerEntry, (void *) witem);
        if (rc) {
            WSLogger::instance()->log("WSScheduler", "Failed to create worker item");
            pthread_attr_destroy(&attr);
            delete witem;
            continue;
        }

        workitempool_->push_back(witem);
    }
}

string WSScheduler::getUniqueFileName(string prefix, string suffix) {
    uuid_t package_uuid;
    uuid_generate_time_safe(package_uuid);

    char str_uuid[40];
    uuid_unparse_lower(package_uuid, str_uuid);

    uuid_clear(package_uuid);

    return prefix + string(str_uuid) + suffix;
}

WSSshSessionPool* WSScheduler::getSSHConnectionPool(WSDHpcCenter& hpcrecord) {
    std::map<int, WSSshSessionPool*>::iterator it = hpcsessionpool_->find(hpcrecord.hpc_id);
    if (it == hpcsessionpool_->end()) {

        string hpc_pass = WSCrypt::instance()->decrypt(hpcrecord.account_password, keydata_, hpcrecord.tsalt1, hpcrecord.tsalt2);

        WSSshSessionPool* sessionpool = new WSSshSessionPool(hpcrecord.account_name, hpc_pass, hpcrecord.address, max_connection_per_hpc_);
        hpcsessionpool_->insert(std::pair<int, WSSshSessionPool*>(hpcrecord.hpc_id, sessionpool));

        return sessionpool;
    }

    return it->second;
}

void WSScheduler::addHPCWork(WSHPCWork* work) {
    pthread_mutex_lock(&mutex_);
    hpcworkqueue_->push_back(work);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
}

WSHPCWork* WSScheduler::removeHPCWork() {
    pthread_mutex_lock(&mutex_);
    while (hpcworkqueue_->size() == 0) {
        pthread_cond_wait(&cond_, &mutex_);
    }

    WSHPCWork* workitem = hpcworkqueue_->front();
    hpcworkqueue_->pop_front();
    pthread_mutex_unlock(&mutex_);

    return workitem;
}

void WSScheduler::runRemoteCommand(ssh_session session, string my_stdin, string* my_stdout) {
    int rc;
    ssh_channel channel;

    channel = ssh_channel_new(session);
    if (channel == NULL) {
        string error = string("failed to create ssh channel") + string(ssh_get_error(session));
        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
    }

    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        ssh_channel_free(channel);
        string error = string("failed to open session") + string(ssh_get_error(session));
        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
    }

    rc = ssh_channel_request_exec(channel, my_stdin.c_str());
    if (rc != SSH_OK) {
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        string error = string("failed to run remote command. error: ") + string(ssh_get_error(session)) + string(" stdin: ") + my_stdin;
        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
    }

    char* buffer = (char*) malloc(stdout_bufsiz_);
    unsigned int nbytes;
    nbytes = ssh_channel_read(channel, buffer, stdout_bufsiz_, 0);

    char* stdout_str = (char*) malloc(nbytes);
    int stdout_index = 0;
    while (nbytes > 0) {
        if (stdout_index > 0)
            stdout_str = (char*) realloc(stdout_str, stdout_index + nbytes);

        memcpy(stdout_str + stdout_index, buffer, nbytes);
        stdout_index += nbytes;
        nbytes = ssh_channel_read(channel, buffer, stdout_bufsiz_, 0);
    }

    *my_stdout = string(stdout_str);

    free(buffer);

    if (nbytes < 0) {
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        string error = string("execution is failed. error: ") + string(ssh_get_error(session)) + string(" stdin: ") + my_stdin;
        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);

    int exit_status = ssh_channel_get_exit_status(channel);
    if (exit_status != 0) {
        ssh_channel_free(channel);
        string error = string("execution is failed to get exit status code. error: ") + string(ssh_get_error(session)) + string(" stdin: ") + my_stdin;
        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
    }

    ssh_channel_free(channel);
}

void WSScheduler::freeWorkflowArray(vector<WSWorkflowTask*>* workflowArray) {
    for (vector<int>::size_type i = 0; i != workflowArray->size(); i++) {
        WSWorkflowTask* workflowitem = workflowArray->at(i);
        delete workflowitem;
    }

    delete workflowArray;
}

bool WSScheduler::readRemoteTextFile(ssh_session session, string path, string* textfile_content, string* error_desc) {
    *textfile_content = "";
    *error_desc = "";

    ssh_scp scp;
    int rc;

    scp = ssh_scp_new
            (session, SSH_SCP_READ, path.c_str());
    if (scp == NULL) {
        *error_desc = string("error allocating scp session: ") + string(ssh_get_error(session));
        return false;
    }

    rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        ssh_scp_free(scp);
        *error_desc = string("error initializing scp session: ") + string(ssh_get_error(session));
        return false;
    }

    rc = ssh_scp_pull_request(scp);
    if (rc != SSH_SCP_REQUEST_NEWFILE) {
        ssh_scp_free(scp);
        *error_desc = string("error in receiving remote file: ") + string(ssh_get_error(session));
        return false;
    }

    int size = ssh_scp_request_get_size(scp);

    ssh_scp_accept_request(scp);

    //TODO: add here maximum allowable buffer size

    char *pChars = new char[size];
    if (!pChars) {
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        *error_desc = string("failed to allocate memory for reading package data");
        return false;
    }

    int nbytes = ssh_scp_read(scp, pChars, size);
    *textfile_content += string(pChars);

    if (nbytes == SSH_ERROR) {
        free(pChars);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        *error_desc = string("error in receiving remote file: ") + string(ssh_get_error(session));
        return false;
    }

    free(pChars);
    ssh_scp_close(scp);
    ssh_scp_free(scp);

    return true;
}

void WSScheduler::parseSQueueForRunningJobs(string& input, map<string, string>& joblist) {
    std::istringstream in(input);

    std::string s;

    std::getline(in, s);
    while (!in.eof()) {
        string job_definition;
        std::getline(in, s);


        std::istringstream ss(s);
        ss >> job_definition;
        if (ss.eof())
            break;

        string job_status;
        for (int i = 0; i < 4; i++)
            ss >> job_status;

        joblist.insert(make_pair(job_definition, job_status));
    }
}

void WSScheduler::parseQStatForRunningJobs(string& input, map<string, string>& joblist) {
    std::istringstream in(input);

    std::string s;

    while (1) {
        string prefix = "---------";
        std::getline(in, s);
        if (in.eof()) {
            return;
        }

        if (s.substr(0, prefix.size()) == prefix) {
            break;
        }
    }

    while (!in.eof()) {
        string job_definition;
        std::getline(in, s);

        std::istringstream ss(s);
        ss >> job_definition;
        if (ss.eof())
            break;

        string job_status;
        for (int i = 0; i < 9; i++)
            ss >> job_status;

        joblist.insert(make_pair(job_definition, job_status));
    }
}

void* WSScheduler::runBackgroundWorker() {
    list<WSDHpcCenter*>* hpclist = new list<WSDHpcCenter*>();
    WSData::instance()->getAllHPCs(true, *hpclist);

    CURL *curl = NULL;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    while (1) {
        sleep(job_status_check_delay_in_seconds_);

        for (std::list<WSDHpcCenter*>::const_iterator hpciterator = hpclist->begin(), hpciteratorend = hpclist->end(); hpciterator != hpciteratorend; ++hpciterator) {
            WSSshSessionPool* sessionpool = NULL;
            WSSshSession* sshsession = NULL;
            list<WSDJobCompact*>* joblist = NULL;
            map<string, string>* joblistonhpc = NULL;

            try {
                WSDHpcCenter* hpcdata = *hpciterator;

                joblist = new list<WSDJobCompact*>();
                WSData::instance()->getAllSubmittedAndRunningJobsByHPCID(true, hpcdata->hpc_id, *joblist);

                if (joblist->size() == 0) {
                    delete joblist;
                    joblist = NULL;
                    continue;
                }

                if (hpcdata->qtype == 1) {
                    // get hpc job status here
                    sessionpool = getSSHConnectionPool(*hpcdata);

                    sshsession = sessionpool->getConnectionFromPool("localhost");

                    string my_stdin = string("qstat -u ") + hpcdata->account_name;
                    string my_stdout = "";

                    runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);

                    joblistonhpc = new map<string, string>();
                    parseQStatForRunningJobs(my_stdout, *joblistonhpc);
                } else if (hpcdata->qtype == 2) {
                    sessionpool = getSSHConnectionPool(*hpcdata);

                    sshsession = sessionpool->getConnectionFromPool("localhost");

                    string my_stdin = string("squeue -u ") + hpcdata->account_name;
                    string my_stdout = "";

                    runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);

                    joblistonhpc = new map<string, string>();
                    parseSQueueForRunningJobs(my_stdout, *joblistonhpc);
                } else {
                    continue;
                }

                for (std::list<WSDJobCompact*>::const_iterator jobiterator = joblist->begin(), jobiteratorend = joblist->end(); jobiterator != jobiteratorend; ++jobiterator) {
                    WSDJobCompact* jobdata = *jobiterator;

                    string callbackurl = jobdata->callbackurl;
                    if (callbackurl != "") {
                        char tempbuffer[15];
                        sprintf(tempbuffer, "%d", jobdata->job_id);
                        string str_id = string(tempbuffer);
                        WSHelper::instance()->replaceStringInPlace(callbackurl, "$id", str_id, true);
                    }

                    map<string, string>::iterator it = joblistonhpc->find(jobdata->hpc_job_desc);

                    if (it == joblistonhpc->end()) {
                        //bool isjobstatechanged;
                        //WSData::instance()->setJobState(true, jobdata->job_id, WSDJobOutputStateTransferInQueue, &isjobstatechanged);
                        // set completed if there exists log file
                        // otherwise set deleted that might be done by admin
                        // do not know whether it is deleted or completed within check delay
                        // let worker thread decide that!
                        /*
                        string jstatus = "";
                        try {
                                jstatus = WSData::instance()->getJobStatus(true, jobdata->job_id);
                         } catch (...) {
                                WSLogger::instance()->log("WSSchedulerBackgroundWorker", "Error in getting job status");
                        }
                        
                        if (jstatus == "InHPCQueue") {
                            continue;
                        }*/

                        bool isjobstatechanged;
                        WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateCompletedInHPC, &isjobstatechanged);

                        if (isjobstatechanged)
                            doCallback(curl, callbackurl, WSDJobStateCompletedInHPC, false);

                        WSHPCWork* workitem = new WSHPCWork();
                        workitem->jobtype = HPCJobCheckResult;
                        workitem->clientip = "localhost";
                        workitem->jobid = jobdata->job_id;
                        workitem->hpcid = jobdata->hpc_id;
                        workitem->packageid = jobdata->package_id;
                        workitem->optional = 1; // means do not know
                        workitem->callbackurl = callbackurl;

                        WSScheduler::instance()->addHPCWork(workitem);
                    } else {
                        if (hpcdata->qtype == 1) {
                            if (it->second == "R") {
                                // set running, if in queue
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateRunning, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateRunning, false);

                            } else if (it->second == "C") {
                                // set completed if running or in queue
                                // start job output file transfer
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateCompletedInHPC, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateCompletedInHPC, false);

                                WSHPCWork* workitem = new WSHPCWork();
                                workitem->jobtype = HPCJobCheckResult;
                                workitem->clientip = "localhost";
                                workitem->jobid = jobdata->job_id;
                                workitem->hpcid = jobdata->hpc_id;
                                workitem->packageid = jobdata->package_id;
                                workitem->optional = 2; // it is known as completed
                                workitem->callbackurl = callbackurl;

                                WSScheduler::instance()->addHPCWork(workitem);
                            } else if (it->second == "E") {
                                // exiting
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateExiting, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateExiting, false);

                            } else if (it->second == "H") {
                                // job is held
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateHeld, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateHeld, false);
                            } else if (it->second == "Q") {
                                // job is in queue on hpc
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateInHPCQueue, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateInHPCQueue, false);
                            } else if (it->second == "T") {
                                // job is moved
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateMoved, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateMoved, false);
                            } else if (it->second == "W") {
                                // job is waiting for its execution time
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateWaiting, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateWaiting, false);
                            } else if (it->second == "S") {
                                // job is suspended
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateSuspended, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateSuspended, false);
                            } else {
                                // parse error, do not change the state
                                WSLogger::instance()->log("WSSchedulerBackgroundWorker", "Error in parsing qstat, Err: " + it->second);
                            }
                        } else if (hpcdata->qtype == 2) {
                            if (it->second == "R") {
                                // set running, if in queue
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateRunning, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateRunning, false);
                            } else if (it->second == "S") {
                                // job is suspended
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateSuspended, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateSuspended, false);
                            } else if (it->second == "CD") {
                                // set completed if running or in queue
                                // start job output file transfer
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateCompletedInHPC, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateCompletedInHPC, false);

                                WSHPCWork* workitem = new WSHPCWork();
                                workitem->jobtype = HPCJobCheckResult;
                                workitem->clientip = "localhost";
                                workitem->jobid = jobdata->job_id;
                                workitem->hpcid = jobdata->hpc_id;
                                workitem->packageid = jobdata->package_id;
                                workitem->optional = 2; // it is known as completed
                                workitem->callbackurl = callbackurl;

                                WSScheduler::instance()->addHPCWork(workitem);
                            } else if (it->second == "CG") {
                                // exiting
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateExiting, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateExiting, false);

                            } else if (it->second == "PD") {
                                // job is waiting for its execution time
                                bool isjobstatechanged;
                                WSData::instance()->setJobState(true, jobdata->job_id, WSDJobStateWaiting, &isjobstatechanged);

                                if (isjobstatechanged)
                                    doCallback(curl, callbackurl, WSDJobStateWaiting, false);
                            } else {
                                // parse error, do not change the state
                                WSLogger::instance()->log("WSSchedulerBackgroundWorker", "Error in parsing squeue, Err: " + it->second);
                            }
                        }
                    }
                }
            } catch (WSException& e) {
                WSLogger::instance()->log("WSSchedulerBackgroundWorker", e);


            } catch (const std::exception& e) {
                WSLogger::instance()->log("WSSchedulerBackgroundWorker", string(e.what()));


            } catch (...) {
                WSLogger::instance()->log("WSSchedulerBackgroundWorker", "Unidentified exception");

            }

            if (joblistonhpc) {
                delete joblistonhpc;
                joblistonhpc = NULL;
            }

            if (joblist) {
                for (std::list<WSDJobCompact*>::const_iterator jobiterator = joblist->begin(), jobiteratorend = joblist->end(); jobiterator != jobiteratorend; ++jobiterator) {
                    WSDJobCompact* jobdata = *jobiterator;


                    delete jobdata;
                }

                delete joblist;
                joblist = NULL;
            }

            if (sessionpool && sshsession)
                sessionpool->returnConnectionToPool(sshsession);
        }
    }

    if (curl)
        curl_easy_cleanup(curl);
}

static size_t null_function(void *buffer, size_t size, size_t nmemb, void *mydata) {
    return size;
}

bool WSScheduler::doCallback(CURL *curl, string url, WSDHPCWorkState state, bool ispackage) {
    if (!curl)
        return false;

    if (url == "")
        return false;

    CURLcode res;

    string str_state = "unknown";
    switch (state) {
        case WSDJobStateError:
            str_state = "JobError";
            break;
        case WSDPackageStateError:
            str_state = "PackageError";
            break;
        case WSDHPCWorkUnknownError:
            str_state = "UnknownError";
            break;
        case WSDPackageStateInQueue:
            str_state = "PackageInServiceQueue";
            break;
        case WSDPackageStateRunning:
            str_state = "PreparingPackage";
            break;
        case WSDPackageStateTransferring:
            str_state = "TransferringPackage";
            break;
        case WSDPackageStateUnzipping:
            str_state = "UnzippingPackage";
            break;
        case WSDPackageStateDone:
            str_state = "PackageTransferDone";
            break;
        case WSDJobStateInServiceQueue:
            str_state = "JobInServiceQueue";
            break;
        case WSDJobStateInHPCQueue:
            str_state = "JobInHPCQueue";
            break;
        case WSDJobStateRunning:
            str_state = "JobRunning";
            break;
        case WSDJobStateExiting:
            str_state = "JobExiting";
            break;
        case WSDJobStateHeld:
            str_state = "JobHeld";
            break;
        case WSDJobStateMoved:
            str_state = "JobMoved";
            break;
        case WSDJobStateWaiting:
            str_state = "JobWaiting";
            break;
        case WSDJobStateSuspended:
            str_state = "JobSuspended";
            break;
        case WSDJobStateCompleted:
            str_state = "JobCompleted";
            break;
        case WSDJobStateDeleted:
            str_state = "JobDeleted";
            break;
        case WSDJobOutputStateTransferInQueue:
            str_state = "JobOutputTransferInQueue";
            break;
        case WSDJobOutputStateTransferring:
            str_state = "JobOutputTransferring";
            break;
        case WSDJobOutputStateTransferDone:
            str_state = "JobOutputTransferDone";
            break;
        case WSDPackageDeleted:
            str_state = "JobPackageDeleted";
            break;
        case WSDJobStateCompletedInHPC:
            str_state = "JobCompletedInHPC";
            break;
    }

    WSHelper::instance()->replaceStringInPlace(url, "$state", str_state, true);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, null_function);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        return false;
    }

    return true;
}

static size_t remoteToSCPWrite(void *buffer, size_t size, size_t nmemb, void *stream) {
    struct RemoteFile *out = (struct RemoteFile *) stream;
    int rc;

    if (out && !out->isinitialized) {
        out->isinitialized = true;

        double filelength;

        curl_easy_getinfo(out->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                &filelength);

        string package_file_name = WSHelper::instance()->getSafeFileName(out->filename);

        rc = ssh_scp_push_file
                (out->scp, "package.zip", filelength, S_IRUSR | S_IWUSR);
        if (rc != SSH_OK) {
            string error = string("can't open remote file: ") + string(ssh_get_error(out->sshsession->session_));
            out->exception = error;
            return -1;
        }
    }

    rc = ssh_scp_write(out->scp, buffer, nmemb * size);
    if (rc != SSH_OK) {
        string error = string("can't write to remote file: ") + string(ssh_get_error(out->sshsession->session_));
        out->exception = error;
        return -1;
    }

    return nmemb * size;
}

static size_t remoteToSCPRead(void *buffer, size_t size, size_t nmemb, void *stream) {
    struct RemoteFile2 *out = (struct RemoteFile2 *) stream;

    size_t nbytes = ssh_scp_read(out->scp, buffer, nmemb * size);
    if (nbytes == SSH_ERROR) {
        string error = string("can't read from remote file: ") + string(ssh_get_error(out->sshsession->session_));
        out->exception = error;
        return -1;
    }

    return nbytes;
}

void* WSScheduler::runWorkerItemEntry(WSScheduler::WSSchedulerWorkItem* witem) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    while (witem->cont) {
        WSHPCWork* workitem = NULL;
        WSSshSession* sshsession = NULL;
        WSSshSessionPool* sessionpool = NULL;
        string my_stdout = "";
        string my_stderr = "";
        string my_stdin = "";
        int error_code = -1;
        CURL *curl = NULL;
        string callbackurl = "";

        try {
            workitem = witem->scheduler->removeHPCWork();
            witem->state = WITEM_PROCESSING;

            if (workitem->jobtype == HPCJobPackageUpload) {
                char tempbuffer [15];

                string callbackurl = workitem->callbackurl;
                if (callbackurl != "") {
                    curl = curl_easy_init();

                    //bool jobcallbackhttps = false;
                    //int pos = callbackurl.find("https");
                    //if (pos == 0)
                    //        jobcallbackhttps = true;

                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                    sprintf(tempbuffer, "%d", workitem->packageid);
                    string str_id = string(tempbuffer);
                    WSHelper::instance()->replaceStringInPlace(callbackurl, "$id", str_id, true);
                }

                WSData::instance()->updatePackageState(true, workitem->packageid, WSDPackageStateRunning);

                doCallback(curl, callbackurl, WSDPackageStateRunning, true);

                WSDHpcCenter hpc_center_rec;
                WSData::instance()->getHpcCenterByID(true, workitem->hpcid, &hpc_center_rec);

                sessionpool = witem->scheduler->getSSHConnectionPool(hpc_center_rec);

                sshsession = sessionpool->getConnectionFromPool(workitem->clientip);

                uuid_t package_uuid;
                uuid_generate_time_safe(package_uuid);

                char str_uuid[37];
                uuid_unparse_lower(package_uuid, str_uuid);

                uuid_clear(package_uuid);

                string remote_dirname = string(str_uuid);

                WSData::instance()->setPackageFolder(true, workitem->packageid, remote_dirname);

                ssh_scp scp;
                int rc;

                scp = ssh_scp_new
                        (sshsession->session_, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, ".");
                if (scp == NULL) {
                    string error = string("error allocating scp session: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_init(scp);
                if (rc != SSH_OK) {
                    ssh_scp_free(scp);
                    string error = string("error initializing scp session: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_push_directory(scp, ".hydrogate", S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: .hydrogate error: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_push_directory(scp, "data", S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: data error: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_push_directory(scp, remote_dirname.c_str(), S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: ") + remote_dirname + " error: " + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                if (isinputinlocalmode_) {
                    ifstream ifs(workitem->packagepath.c_str(), ios::binary | ios::ate);

                    if (!ifs.good()) {
                        ifs.close();
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        string error = string("couldn't find the file: ") + workitem->packagepath;
                        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                    }

                    ifstream::pos_type length = ifs.tellg();

                    ifs.seekg(0, ios::beg);

                    char *pChars = new char[scp_bufsiz_];
                    if (!pChars) {
                        ifs.close();
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        string error = string("failed to allocate memory for reading package data");
                        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                    }

                    try {
                        WSData::instance()->updatePackageState(true, workitem->packageid, WSDPackageStateTransferring);
                    } catch (WSException& e) {
                        ifs.close();
                        free(pChars);
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        throw;
                    }

                    doCallback(curl, callbackurl, WSDPackageStateTransferring, true);

                    rc = ssh_scp_push_file
                            (scp, "package.zip", length, S_IRUSR | S_IWUSR);
                    if (rc != SSH_OK) {
                        ifs.close();
                        free(pChars);
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        string error = string("can't open remote file: ") + string(ssh_get_error(sshsession->session_));
                        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                    }

                    int percentagedone = 0;
                    size_t uploadedsofar = 0;

                    while (true) {
                        ifs.read(pChars, scp_bufsiz_);

                        int numofreadbytes = ifs.gcount();
                        uploadedsofar += numofreadbytes;
                        percentagedone = (int) ((((double) uploadedsofar) / ((double) length)) * 100.0);

                        if (numofreadbytes <= 0)
                            break;

                        rc = ssh_scp_write(scp, pChars, numofreadbytes);
                        if (rc != SSH_OK) {
                            ifs.close();
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("can't write to remote file: ") + string(ssh_get_error(sshsession->session_));
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }
                    }

                    ifs.close();
                    free(pChars);
                } else {
                    try {
                        WSData::instance()->updatePackageState(true, workitem->packageid, WSDPackageStateTransferring);
                    } catch (WSException& e) {
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        throw;
                    }

                    doCallback(curl, callbackurl, WSDPackageStateTransferring, true);

                    bool isIrods = false;
                    if (workitem->packagepath[0] == '/') {
                        isIrods = true;
                    }

                    if (isIrods) {
                        string pathinlocal = getUniqueFileName(workitem->inputfolder, ".zip");
                        string command = "iget " + workitem->packagepath + " " + pathinlocal;

                        int ret = system(command.c_str());

                        if (ret != 0) {
                            string error = string("failed to iget the file ") + command;
                            throw WSException(WS_EXCEPTION_IRODS, error);
                        }

                        ifstream ifs(pathinlocal.c_str(), ios::binary | ios::ate);

                        if (!ifs.good()) {
                            ifs.close();
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("couldn't find the file: ") + pathinlocal;
                            remove(pathinlocal.c_str());
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }

                        ifstream::pos_type length = ifs.tellg();

                        ifs.seekg(0, ios::beg);

                        char *pChars = new char[scp_bufsiz_];
                        if (!pChars) {
                            ifs.close();
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            remove(pathinlocal.c_str());
                            string error = string("failed to allocate memory for reading package data");
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }

                        try {
                            WSData::instance()->updatePackageState(true, workitem->packageid, WSDPackageStateTransferring);
                        } catch (WSException& e) {
                            ifs.close();
                            remove(pathinlocal.c_str());
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            throw;
                        }

                        doCallback(curl, callbackurl, WSDPackageStateTransferring, true);

                        rc = ssh_scp_push_file
                                (scp, "package.zip", length, S_IRUSR | S_IWUSR);
                        if (rc != SSH_OK) {
                            ifs.close();
                            remove(pathinlocal.c_str());
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("can't open remote file: ") + string(ssh_get_error(sshsession->session_));
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }

                        int percentagedone = 0;
                        size_t uploadedsofar = 0;

                        while (true) {
                            ifs.read(pChars, scp_bufsiz_);

                            int numofreadbytes = ifs.gcount();
                            uploadedsofar += numofreadbytes;
                            percentagedone = (int) ((((double) uploadedsofar) / ((double) length)) * 100.0);

                            if (numofreadbytes <= 0)
                                break;

                            rc = ssh_scp_write(scp, pChars, numofreadbytes);
                            if (rc != SSH_OK) {
                                remove(pathinlocal.c_str());
                                ifs.close();
                                free(pChars);
                                ssh_scp_close(scp);
                                ssh_scp_free(scp);
                                string error = string("can't write to remote file: ") + string(ssh_get_error(sshsession->session_));
                                throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                            }
                        }

                        remove(pathinlocal.c_str());
                        ifs.close();
                        free(pChars);
                    } else {
                        CURL *transfercurl;
                        CURLcode res;

                        transfercurl = curl_easy_init();

                        struct RemoteFile remotefile = {workitem->packagepath, transfercurl, scp, false, sshsession, ""};

                        curl_easy_setopt(transfercurl, CURLOPT_URL, workitem->packagepath.c_str());
                        curl_easy_setopt(transfercurl, CURLOPT_WRITEFUNCTION, remoteToSCPWrite);
                        curl_easy_setopt(transfercurl, CURLOPT_BUFFERSIZE, scp_bufsiz_);
                        curl_easy_setopt(transfercurl, CURLOPT_WRITEDATA, &remotefile);
                        curl_easy_setopt(transfercurl, CURLOPT_VERBOSE, 0);

                        res = curl_easy_perform(transfercurl);

                        curl_easy_cleanup(transfercurl);

                        if (CURLE_OK != res) {
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            throw WSException(WS_EXCEPTION_SSH_ERROR, remotefile.exception);
                        }
                    }
                }

                ssh_scp_close(scp);
                ssh_scp_free(scp);

                WSData::instance()->updatePackageState(true, workitem->packageid, WSDPackageStateDone);

                doCallback(curl, callbackurl, WSDPackageStateDone, true);
            } else if (workitem->jobtype == HPCJobSubmit) {
                char tempbuffer[30];
                string callbackurl = workitem->callbackurl;

                if (callbackurl != "") {
                    curl = curl_easy_init();
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                    sprintf(tempbuffer, "%d", workitem->jobid);
                    string str_id = string(tempbuffer);
                    WSHelper::instance()->replaceStringInPlace(callbackurl, "$id", str_id, true);
                }

                WSDHpcCenter hpc_center_rec;
                WSData::instance()->getHpcCenterByID(true, workitem->hpcid, &hpc_center_rec);

                //TODO: check null pointer
                sessionpool = witem->scheduler->getSSHConnectionPool(hpc_center_rec);

                sshsession = sessionpool->getConnectionFromPool(workitem->clientip);

                string remote_dirname = workitem->packagefoldername;

                ssh_scp scp;
                int rc;

                scp = ssh_scp_new
                        (sshsession->session_, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, ".");
                if (scp == NULL) {
                    string error = string("error allocating scp session: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_init(scp);
                if (rc != SSH_OK) {
                    ssh_scp_free(scp);
                    string error = string("error initializing scp session: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_push_directory(scp, ".hydrogate", S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: .hydrogate error: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_push_directory(scp, "data", S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: data error: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_push_directory(scp, remote_dirname.c_str(), S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: ") + remote_dirname + " error: " + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                sprintf(tempbuffer, "JOB_%d", workitem->jobid);
                string jobfolder = string(tempbuffer);

                rc = ssh_scp_push_directory(scp, jobfolder.c_str(), S_IRWXU);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't create remote directory: ") + jobfolder + " error: " + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                // unzip package.zip into job folder
                string cd_command = string("cd ") + string("~/.hydrogate/data/") + remote_dirname + string(" && ");
                my_stdin = cd_command + string("unzip -o package.zip -d ") + jobfolder;
                runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);

                WSDHPCTemplate hpc_template;
                WSData::instance()->getHPCTemplateByID(true, workitem->hpcid, &hpc_template);

                string jobname = jobfolder;
                string jobuser = hpc_center_rec.account_name;
                string walltime = workitem->walltime;
                sprintf(tempbuffer, "%d", workitem->nodes);
                string nodes = string(tempbuffer);
                sprintf(tempbuffer, "%d", workitem->ppn);
                string ppn = string(tempbuffer);

                sprintf(tempbuffer, "result_job%d.zip", workitem->jobid);
                string resultzipfilename = string(tempbuffer);

                string pbs_template = "";

                if (workitem->isworkflow) {
                    string script_header = hpc_template.pbs_script_header;
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_JOB_NAME", jobname, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_ERROR_LOG_FILE", "stderr.log", false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_OUTPUT_LOG_FILE", "stdout.log", false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_JOB_USER", jobuser, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_NODES", nodes, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_PPN", ppn, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_WALLTIME", walltime, false);

                    string script_tasks = "";
                    for (vector<int>::size_type i = 0; i != workitem->workflow_tasks->size(); i++) {
                        WSWorkflowTask* workflowitem = workitem->workflow_tasks->at(i);

                        string task_command = hpc_template.pbs_script_task;

                        string runner = "";
                        string runnerparam = "";
                        string program = workflowitem->program_path;
                        string programparam = workflowitem->processparameters;

                        if (workflowitem->program_type == HPCProcessMPI) {
                            runner = "mpiexec";
                            int numofprocs = workflowitem->nodes * workflowitem->ppn;
                            sprintf(tempbuffer, "%d", numofprocs);
                            runnerparam = string("-n ") + string(tempbuffer);
                        } else if (workflowitem->program_type == HPCProcessBashScript) {
                            runner = "bash";
                        }

                        WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_RUNNER", runner, false);
                        WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_RUNNER_PARAM", runnerparam, false);
                        WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_PROGRAM", program, false);
                        WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_PROGRAM_PARAM", programparam, false);
                        WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_ERRORCODE_FILE", "errorcode.log", false);

                        script_tasks += task_command + "\n";
                    }

                    string script_tail = hpc_template.pbs_script_tail;
                    WSHelper::instance()->replaceStringInPlace(script_tail, "$HGW_ERRORCODE_FILE", "errorcode.log", false);
                    WSHelper::instance()->replaceStringInPlace(script_tail, "$HGW_RESULT_ZIP_FILE_NAME", resultzipfilename, false);
                    workitem->outputlist += " stderr.log  stdout.log";
                    WSHelper::instance()->replaceStringInPlace(script_tail, "$HGW_RESULT_FILE_LIST", workitem->outputlist, false);

                    pbs_template = script_header + "\n\n" + script_tasks + "\n" + script_tail;
                } else {
                    string runner = "";
                    string runnerparam = "";
                    string program = workitem->programpath;
                    string programparam = workitem->processparameters;

                    if (workitem->programtype == HPCProcessMPI) {
                        runner = "mpiexec";
                        int numofprocs = workitem->nodes * workitem->ppn;
                        sprintf(tempbuffer, "%d", numofprocs);
                        runnerparam = string("-n ") + string(tempbuffer);
                    } else if (workitem->programtype == HPCProcessBashScript) {
                        runner = "bash";
                    }

                    string script_header = hpc_template.pbs_script_header;
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_JOB_NAME", jobname, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_ERROR_LOG_FILE", "stderr.log", false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_OUTPUT_LOG_FILE", "stdout.log", false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_JOB_USER", jobuser, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_NODES", nodes, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_PPN", ppn, false);
                    WSHelper::instance()->replaceStringInPlace(script_header, "$HGW_WALLTIME", walltime, false);

                    string task_command = hpc_template.pbs_script_task;
                    WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_RUNNER", runner, false);
                    WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_RUNNER_PARAM", runnerparam, false);
                    WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_PROGRAM", program, false);
                    WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_PROGRAM_PARAM", programparam, false);
                    WSHelper::instance()->replaceStringInPlace(task_command, "$HGW_ERRORCODE_FILE", "errorcode.log", false);

                    string script_tail = hpc_template.pbs_script_tail;
                    WSHelper::instance()->replaceStringInPlace(script_tail, "$HGW_ERRORCODE_FILE", "errorcode.log", false);
                    WSHelper::instance()->replaceStringInPlace(script_tail, "$HGW_RESULT_ZIP_FILE_NAME", resultzipfilename, false);
                    workitem->outputlist += " stderr.log  stdout.log";
                    WSHelper::instance()->replaceStringInPlace(script_tail, "$HGW_RESULT_FILE_LIST", workitem->outputlist, false);

                    pbs_template = script_header + "\n\n" + task_command + "\n\n" + script_tail;
                }

                sprintf(tempbuffer, "script_job%d.sh", workitem->jobid);
                string pbs_script_file_name = string(tempbuffer);

                rc = ssh_scp_push_file
                        (scp, pbs_script_file_name.c_str(), pbs_template.length(), S_IRUSR | S_IWUSR);
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't open remote job script file: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_write(scp, pbs_template.c_str(), pbs_template.length());
                if (rc != SSH_OK) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("can't write to remote job script file: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                ssh_scp_close(scp);
                ssh_scp_free(scp);

                cd_command = string("cd ") + string("~/.hydrogate/data/") + remote_dirname + string("/") + jobfolder + string("/") + string(" && ");
                if (hpc_center_rec.qtype == 1)
                    my_stdin = cd_command + string("qsub ") + pbs_script_file_name;
                else if (hpc_center_rec.qtype == 2)
                    my_stdin = cd_command + string("sbatch ") + pbs_script_file_name;

                runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);

                // give some time for the scheduler to get the job
                //sleep(10);

                /*
                string input = hpc_template.pbs_qsub_jobname_location;
                size_t pos = 0;
                std::string firstnumber;
                if ((pos = input.find(":")) != std::string::npos) {
                    firstnumber = input.substr(0, pos);
                    input.erase(0, pos + 1);
                } else {
                    throw WSException(WS_EXCEPTION_BAD_PARAM, "Error in parsing pbs_qsub_jobname_location. Delimiter ':' not found. HPC: " + hpc_center_rec.name);
                }

                char *end;
                const char* tbuf = firstnumber.c_str();
                int row = strtol(tbuf, &end, 10);
                if (end == tbuf)
                    throw WSException(WS_EXCEPTION_BAD_PARAM, "Error in parsing pbs_qsub_jobname_location. First token is not a number. HPC: " + hpc_center_rec.name);

                tbuf = input.c_str();
                int column = strtol(tbuf, &end, 10);
                if (end == tbuf)
                    throw WSException(WS_EXCEPTION_BAD_PARAM, "Error in parsing pbs_qsub_jobname_location. Second token is not a number. HPC: " + hpc_center_rec.name);

                string job_description = WSHelper::instance()->extractStringByRowAndColumn(my_stdout, row, column);
                 */

                string job_description = " ";
                string hpc_job_id = " ";
                
                if (hpc_center_rec.qtype == 1) {
                    job_description = WSHelper::instance()->extractStringByRowAndColumn(my_stdout, 1, 1);
                    size_t pos = 0;
                    if ((pos = job_description.find(".")) != std::string::npos) {
                        hpc_job_id = job_description.substr(0, pos);
                    } else {
                        throw WSException(WS_EXCEPTION_BAD_PARAM, "Error in parsing jobdescription. HPC: " + hpc_center_rec.name);
                    }
                } else if (hpc_center_rec.qtype == 2) {
                    job_description = WSHelper::instance()->extractStringByRowAndColumn(my_stdout, 1, 4);
                    hpc_job_id = job_description;
                }

                WSData::instance()->setJobSubmittedByID(true, workitem->jobid, hpc_job_id, job_description);

                doCallback(curl, callbackurl, WSDJobStateInHPCQueue, false);
            } else if (workitem->jobtype == HPCJobCheckResult) {
                workitem->errorcode = 1;

                char tempbuff[100];
                string callbackurl = workitem->callbackurl;

                if (callbackurl != "") {
                    curl = curl_easy_init();
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                    sprintf(tempbuff, "%d", workitem->jobid);
                    string str_id = string(tempbuff);
                    WSHelper::instance()->replaceStringInPlace(callbackurl, "$id", str_id, true);
                }

                WSDHpcCenter hpc_center_rec;
                WSData::instance()->getHpcCenterByID(true, workitem->hpcid, &hpc_center_rec);

                //TODO: check null pointer
                sessionpool = witem->scheduler->getSSHConnectionPool(hpc_center_rec);

                sshsession = sessionpool->getConnectionFromPool(workitem->clientip);

                string remote_dirname;
                WSData::instance()->getPackageFolderInformationByID(true, workitem->packageid, NULL, NULL, &remote_dirname);

                sprintf(tempbuff, "JOB_%d", workitem->jobid);

                string job_dirname = string(tempbuff);

                sprintf(tempbuff, "result_job%d.zip", workitem->jobid);
                string resultzipfilename = string(tempbuff);

                string errorcodefilepath = "~/.hydrogate/data/" + remote_dirname + "/" + job_dirname + "/" + "errorcode.log";
                string stdoutfilepath = "~/.hydrogate/data/" + remote_dirname + "/" + job_dirname + "/" + "stdout.log";
                string stderrfilepath = "~/.hydrogate/data/" + remote_dirname + "/" + job_dirname + "/" + "stderr.log";

                string errortextfilecontent;
                string errordescription;

                bool isok = readRemoteTextFile(sshsession->session_, errorcodefilepath, &errortextfilecontent, &errordescription);

                if (!isok) {
                    string error = string("error in reading error file: ") + errorcodefilepath + " message: " + errordescription;
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                sscanf(errortextfilecontent.c_str(), "%d", &error_code);

                /*
                // stdout and stderr log files are contained in the zip file
                isok = readRemoteTextFile(sshsession->session_, stdoutfilepath, &my_stdout, &errordescription);

                if (!isok) {
                    string error = string("error in reading stdout file: ") + stdoutfilepath + " message: " + errordescription;
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                isok = readRemoteTextFile(sshsession->session_, stderrfilepath, &my_stderr, &errordescription);

                if (!isok) {
                    string error = string("error in reading stderr file: ") + stdoutfilepath + " message: " + errordescription;
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }*/

                my_stdout = " ";
                my_stderr = " ";

                sprintf(tempbuff, "result_job%d.zip", workitem->jobid);

                ssh_scp scp;
                int rc;

                string resultzipfilepath = "~/.hydrogate/data/" + remote_dirname + "/" + job_dirname + "/" + resultzipfilename;

                scp = ssh_scp_new
                        (sshsession->session_, SSH_SCP_READ, resultzipfilepath.c_str());
                if (scp == NULL) {
                    string error = string("error allocating scp session: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_init(scp);
                if (rc != SSH_OK) {
                    ssh_scp_free(scp);
                    string error = string("error initializing scp session: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                rc = ssh_scp_pull_request(scp);
                if (rc != SSH_SCP_REQUEST_NEWFILE) {
                    workitem->errorcode = 2;
                    ssh_scp_free(scp);
                    string error = string("error in receiving remote file: ") + string(ssh_get_error(sshsession->session_));
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                size_t resultfilesize = ssh_scp_request_get_size(scp);

                ssh_scp_accept_request(scp);

                WSData::instance()->setJobState(true, workitem->jobid, WSDJobOutputStateTransferring, NULL);

                doCallback(curl, callbackurl, WSDJobOutputStateTransferring, false);

                char *pChars = new char[scp_bufsiz_];
                if (!pChars) {
                    ssh_scp_close(scp);
                    ssh_scp_free(scp);
                    string error = string("failed to allocate memory for reading package data");
                    throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                }

                if (isoutputinlocalmode_) {
                    //TODO: if is local its local path, otherwise it's ftp
                    string localresultzippath = outputfolder_ + resultzipfilename;

                    ofstream ofs(localresultzippath.c_str(), ios::binary | ios::out);

                    if (!ofs.is_open() || !ofs.good()) {
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        string error = string("couldn't open the file for reading: ") + localresultzippath;
                        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                    }

                    size_t expectednbytes = scp_bufsiz_;
                    if (resultfilesize <= scp_bufsiz_) {
                        expectednbytes = resultfilesize;
                    }

                    size_t nbytes = ssh_scp_read(scp, pChars, expectednbytes);
                    size_t receivednbytes = 0;

                    if (nbytes == SSH_ERROR) {
                        ofs.close();
                        free(pChars);
                        ssh_scp_close(scp);
                        ssh_scp_free(scp);
                        string error = string("error in receiving remote file: ") + string(ssh_get_error(sshsession->session_));
                        throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                    }

                    while (true) {
                        receivednbytes += nbytes;

                        ofs.write(pChars, nbytes);
                        if (ofs.bad()) {
                            ofs.close();
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("couldn't write the local result file: ") + localresultzippath;
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }

                        if (receivednbytes == resultfilesize)
                            break;

                        int expectednbytes = scp_bufsiz_;
                        if ((resultfilesize - receivednbytes) <= scp_bufsiz_) {
                            expectednbytes = resultfilesize - receivednbytes;
                        }

                        nbytes = ssh_scp_read(scp, pChars, expectednbytes);

                        if (nbytes == SSH_ERROR) {
                            ofs.close();
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("error in receiving remote file: ") + string(ssh_get_error(sshsession->session_));
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }
                    }

                    ofs.close();
                } else {
                    // its irods address, outputurlsrc_ also defines output path contrary to outputurl_ in curl
                    if (outputurlsrc_[0] == '/') {
                        // get file to local
                        string localresultzippath = outputfolder_ + resultzipfilename;

                        ofstream ofs(localresultzippath.c_str(), ios::binary | ios::out);

                        if (!ofs.is_open() || !ofs.good()) {
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("couldn't open the file for reading: ") + localresultzippath;
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }

                        size_t expectednbytes = scp_bufsiz_;
                        if (resultfilesize <= scp_bufsiz_) {
                            expectednbytes = resultfilesize;
                        }

                        size_t nbytes = ssh_scp_read(scp, pChars, expectednbytes);
                        size_t receivednbytes = 0;

                        if (nbytes == SSH_ERROR) {
                            ofs.close();
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("error in receiving remote file: ") + string(ssh_get_error(sshsession->session_));
                            throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                        }

                        while (true) {
                            receivednbytes += nbytes;

                            ofs.write(pChars, nbytes);
                            if (ofs.bad()) {
                                ofs.close();
                                free(pChars);
                                ssh_scp_close(scp);
                                ssh_scp_free(scp);
                                string error = string("couldn't write the local result file: ") + localresultzippath;
                                throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                            }

                            if (receivednbytes == resultfilesize)
                                break;

                            int expectednbytes = scp_bufsiz_;
                            if ((resultfilesize - receivednbytes) <= scp_bufsiz_) {
                                expectednbytes = resultfilesize - receivednbytes;
                            }

                            nbytes = ssh_scp_read(scp, pChars, expectednbytes);

                            if (nbytes == SSH_ERROR) {
                                ofs.close();
                                free(pChars);
                                ssh_scp_close(scp);
                                ssh_scp_free(scp);
                                string error = string("error in receiving remote file: ") + string(ssh_get_error(sshsession->session_));
                                throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                            }
                        }

                        ofs.close();

                        // upload it to irods, first create folder with job name
                        sprintf(tempbuff, "%d/", workitem->jobid);
                        string resultdirname = string(tempbuff);
                        string command = "imkdir -p " + outputurlsrc_ + resultdirname;

                        int ret = system(command.c_str());

                        if (ret != 0) {
                            remove(localresultzippath.c_str());
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("failed to imkdir ") + command;
                            throw WSException(WS_EXCEPTION_IRODS, error);
                        }

                        // upload the file to irods
                        command = "iput " + localresultzippath + " " + outputurlsrc_ + resultdirname + "result.zip";

                        ret = system(command.c_str());

                        if (ret != 0) {
                            remove(localresultzippath.c_str());
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);
                            string error = string("failed to iput the file ") + command;
                            throw WSException(WS_EXCEPTION_IRODS, error);
                        }

                        remove(localresultzippath.c_str());
                    } else { // its curl source
                        CURL *transfercurl;
                        CURLcode res;

                        transfercurl = curl_easy_init();

                        sprintf(tempbuff, "%d/result.zip", workitem->jobid);
                        string resultzipfilename = string(tempbuff);
                        string remoteresultzippath = outputurl_ + resultzipfilename;

                        struct RemoteFile2 remotefile = {scp, sshsession, ""};

                        curl_easy_setopt(transfercurl, CURLOPT_SSL_VERIFYPEER, 0L);
                        curl_easy_setopt(transfercurl, CURLOPT_SSL_VERIFYHOST, 0L);
                        curl_easy_setopt(transfercurl, CURLOPT_READFUNCTION, remoteToSCPRead);
                        curl_easy_setopt(transfercurl, CURLOPT_UPLOAD, 1L);
                        curl_easy_setopt(transfercurl, CURLOPT_URL, remoteresultzippath.c_str());
                        curl_easy_setopt(transfercurl, CURLOPT_READDATA, &remotefile);
                        curl_easy_setopt(transfercurl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) resultfilesize);
                        curl_easy_setopt(transfercurl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR);

                        res = curl_easy_perform(transfercurl);

                        if (CURLE_OK != res) {
                            free(pChars);
                            ssh_scp_close(scp);
                            ssh_scp_free(scp);

                            remotefile.exception += " curlret: " + string(curl_easy_strerror(res));

                            curl_easy_cleanup(transfercurl);

                            throw WSException(WS_EXCEPTION_SSH_ERROR, remotefile.exception + " - File: " + remoteresultzippath);
                        }

                        curl_easy_cleanup(transfercurl);
                    }
                }

                ssh_scp_close(scp);
                ssh_scp_free(scp);
                free(pChars);

                if (error_code != 0) {
                    sprintf(tempbuff, "%d", error_code);
                    string error = string("program execution is failed. error code: ") + string(tempbuff);
                    //throw WSException(WS_EXCEPTION_SSH_ERROR, error);
                    WSData::instance()->setJobFailedByID(true, workitem->jobid, error, my_stderr, error_code);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } else {
                    //WSData::instance()->setJobState(true, workitem->jobid, WSDJobOutputStateTransferDone, NULL);
                    WSData::instance()->setJobDoneByID(true, workitem->jobid, my_stderr, my_stdout, error_code);
                    doCallback(curl, callbackurl, WSDJobOutputStateTransferDone, false);
                }
            } else if (workitem->jobtype == HPCJobDelete) {
                char tempbuffer[30];

                string callbackurl = workitem->callbackurl;
                if (callbackurl != "") {
                    curl = curl_easy_init();
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                    sprintf(tempbuffer, "%d", workitem->jobid);
                    string str_id = string(tempbuffer);
                    WSHelper::instance()->replaceStringInPlace(callbackurl, "$id", str_id, true);
                }

                WSDHpcCenter hpc_center_rec;
                WSData::instance()->getHpcCenterByID(true, workitem->hpcid, &hpc_center_rec);

                //TODO: check null pointer
                sessionpool = witem->scheduler->getSSHConnectionPool(hpc_center_rec);

                sshsession = sessionpool->getConnectionFromPool(workitem->clientip);

                WSData::instance()->deleteJobByID(false, workitem->jobid);

                doCallback(curl, callbackurl, WSDJobStateDeleted, false);

                char jobidbuffer [15];
                sprintf(jobidbuffer, "%d", workitem->hpcjobid);

                // call qdel
                string my_stdin = "qdel " + string(jobidbuffer);
                string my_stdout;
                runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);

                // rm job folder
                string remote_dirname;
                WSData::instance()->getPackageFolderInformationByID(true, workitem->packageid, NULL, NULL, &remote_dirname);

                char tempbuff[30];
                sprintf(tempbuff, "JOB_%d", workitem->jobid);
                string job_dirname = string(tempbuff);

                my_stdin = string("rm -rf ") + string("~/.hydrogate/data/") + remote_dirname + string("/") + job_dirname;
                runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);
            } else if (workitem->jobtype == HPCPackageDelete) {
                char tempbuffer[30];
                string callbackurl = workitem->callbackurl;

                if (callbackurl != "") {
                    curl = curl_easy_init();
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

                    sprintf(tempbuffer, "%d", workitem->packageid);
                    string str_id = string(tempbuffer);
                    WSHelper::instance()->replaceStringInPlace(callbackurl, "$id", str_id, true);
                }

                WSDHpcCenter hpc_center_rec;
                WSData::instance()->getHpcCenterByID(true, workitem->hpcid, &hpc_center_rec);

                //TODO: check null pointer
                sessionpool = witem->scheduler->getSSHConnectionPool(hpc_center_rec);

                sshsession = sessionpool->getConnectionFromPool(workitem->clientip);

                doCallback(curl, callbackurl, WSDPackageDeleted, true);

                string my_stdin = string("rm -rf ") + string("~/.hydrogate/data/") + workitem->packagefoldername;
                runRemoteCommand(sshsession->session_, my_stdin, &my_stdout);
            }
        } catch (WSException& e) {
            WSLogger::instance()->log("WSScheduler", workitem->clientip, e);

            if (workitem->jobtype == HPCJobPackageUpload) {
                try {
                    WSData::instance()->updatePackageError(true, workitem->packageid, e.exceptionString(), my_stdin, my_stdout);
                    doCallback(curl, callbackurl, WSDPackageStateError, true);
                } catch (...) {
                }
            } else if (workitem->jobtype == HPCJobSubmit) {
                try {
                    WSData::instance()->setJobErrorByID(true, workitem->jobid, e.exceptionString(), my_stdin, my_stdout);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } catch (...) {
                }
            } else if (workitem->jobtype == HPCJobCheckResult) {
                try {
                    WSData::instance()->setJobFailedByID(true, workitem->jobid, e.exceptionString(), my_stderr, error_code);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } catch (...) {
                }
            }
        } catch (const std::exception& e) {
            WSLogger::instance()->log("WSScheduler", workitem->clientip, string(e.what()));

            if (workitem->jobtype == HPCJobPackageUpload) {
                try {
                    WSData::instance()->updatePackageError(true, workitem->packageid, string(e.what()), my_stdin, my_stdout);
                    doCallback(curl, callbackurl, WSDPackageStateError, true);
                } catch (...) {
                }
            } else if (workitem->jobtype == HPCJobSubmit) {
                try {
                    WSData::instance()->setJobErrorByID(true, workitem->jobid, string(e.what()), my_stdin, my_stdout);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } catch (...) {
                }
            } else if (workitem->jobtype == HPCJobCheckResult) {
                try {
                    WSData::instance()->setJobFailedByID(true, workitem->jobid, string(e.what()), my_stderr, error_code);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } catch (...) {
                }
            }
        } catch (...) {
            WSLogger::instance()->log("WSScheduler", workitem->clientip, "Unidentified exception");

            if (workitem->jobtype == HPCJobPackageUpload) {
                try {
                    WSData::instance()->updatePackageError(true, workitem->packageid, "Unidentified exception", my_stdin, my_stdout);
                    doCallback(curl, callbackurl, WSDPackageStateError, true);
                } catch (...) {
                }
            } else if (workitem->jobtype == HPCJobSubmit) {
                try {
                    WSData::instance()->setJobErrorByID(true, workitem->jobid, "Unidentified exception", my_stdin, my_stdout);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } catch (...) {
                }
            } else if (workitem->jobtype == HPCJobCheckResult) {
                try {
                    WSData::instance()->setJobFailedByID(true, workitem->jobid, "Unidentified exception", my_stderr, error_code);
                    doCallback(curl, callbackurl, WSDJobStateError, false);
                } catch (...) {
                }
            }
        }

        if (curl)
            curl_easy_cleanup(curl);

        delete workitem;

        if (sshsession)
            sessionpool->returnConnectionToPool(sshsession);

        witem->state = WITEM_WAITING;
    }
}
