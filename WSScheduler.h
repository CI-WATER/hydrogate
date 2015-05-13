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

#ifndef WSSCHEDULER_H
#define	WSSCHEDULER_H

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <set>
#include <pthread.h>
#include "WSSshSessionPool.h"
#include "WSData.h"
#include "WSParameter.h"
#include "WSLogger.h"
#include <curl/curl.h>

using namespace std;

struct WSHPCWork;

enum HPCJobType {
    HPCJobPackageUpload,
    HPCJobSubmit,
    HPCJobStatus,
    HPCJobCheckResult,
    HPCJobDelete,
    HPCPackageDelete
};

enum HPCProgramType {
    HPCProcessSequential = 1,
    HPCProcessMPI = 2,
    HPCProcessBashScript = 3
};

struct WSWorkflowTask {
    string program_name;
    HPCProgramType program_type;
    string processparameters;
    string program_path;
    int nodes;
    int ppn;
};

class WSScheduler {
    friend struct WSHPCWork;
    friend class WSProxy;

public:
    WSScheduler();
    virtual ~WSScheduler();
    static WSScheduler* instance();

    void initialize();
    void addHPCWork(WSHPCWork* work);
    WSHPCWork* removeHPCWork();
    string getUniqueFileName(string prefix, string suffix);
    void freeWorkflowArray(vector<WSWorkflowTask*>* workflowArray);
    
    string getOutputFolder() const {
        return outputfolder_;
    }
    
    string getOutputURL() const {
        return outputurl_;
    }
    
    string getOutputURLSrc() const {
        return outputurlsrc_;
    }

private:

    enum WorkItemState {
        WITEM_WAITING,
        WITEM_PROCESSING
    };

    struct WSSchedulerWorkItem {
        WSScheduler* scheduler;
        pthread_t job_submit_thread;
        int thread_num;
        volatile bool cont;
        volatile WorkItemState state;
    };

    static WSScheduler* instance_;
    static int thread_num_;

    pthread_t background_worker_thread_;
    pthread_mutex_t mutex_;
    pthread_mutex_t mutex_cpool_;
    pthread_cond_t cond_;
    string keydata_;
    map<int, WSSshSessionPool*>* hpcsessionpool_;
    list<WSScheduler::WSSchedulerWorkItem*>* workitempool_;
    list<WSHPCWork*>* hpcworkqueue_;
    unsigned int initial_thread_count_;
    unsigned int max_connection_per_hpc_;
    unsigned int scp_bufsiz_;
    unsigned int stdout_bufsiz_;
    unsigned int job_status_check_delay_in_seconds_;
    string outputfolder_;
    bool isinputinlocalmode_;
    bool isoutputinlocalmode_;
    string inputurl_;
    string outputurl_;
    string outputurlsrc_;
    bool isabsoluteurl_;
        
    void setIsAbsoluteURL(bool isabsoluteurl) {
        isabsoluteurl_ = isabsoluteurl;
    }

    void setInputLocalMode(bool localmode) {
        isinputinlocalmode_ = localmode;
    }
    
    void setOutputLocalMode(bool localmode) {
        isoutputinlocalmode_ = localmode;
    }

    void setInputURL(string inputurl) {
        inputurl_ = inputurl;
    }

    void setOutputURL(string outputurl) {
        if (outputurl[outputurl.length() - 1] != '/') {
            outputurl_ = outputurl + "/";
        } else {
            outputurl_ = outputurl;
        }
    }
    
    void setOutputURLSrc(string outputurlsrc) {
        if (outputurlsrc[outputurlsrc.length() - 1] != '/') {
            outputurlsrc_ = outputurlsrc + "/";
        } else {
            outputurlsrc_ = outputurlsrc;
        }
    }

    void setKeydata(string keydata) {
        keydata_ = keydata;
    }

    void setInitialThreadCount(unsigned int initial_thread_count) {
        if (initial_thread_count > 0)
            initial_thread_count_ = initial_thread_count;
    }

    void setMaxConnectionPerHPC(unsigned int max_connection_per_hpc) {
        if (max_connection_per_hpc > 0)
            max_connection_per_hpc_ = max_connection_per_hpc;
    }

    void setSCPBufsiz(unsigned int scp_bufsiz) {
        if (scp_bufsiz > 0)
            scp_bufsiz_ = scp_bufsiz;
    }

    void setStdoutBufsiz(unsigned int stdout_bufsiz) {
        if (stdout_bufsiz > 0)
            stdout_bufsiz_ = stdout_bufsiz;
    }

    void setJobStatusCheckDelayInSeconds(unsigned int job_status_check_delay_in_seconds) {
        if (job_status_check_delay_in_seconds > 0)
            job_status_check_delay_in_seconds_ = job_status_check_delay_in_seconds;
    }

    void setOutputFolder(string outputfolder) {
        if (outputfolder[outputfolder.length() - 1] != '/') {
            outputfolder = outputfolder + "/";
        }
        outputfolder_ = outputfolder;
    }

    WSSshSessionPool* getSSHConnectionPool(WSDHpcCenter& hpcrecord);
    void runRemoteCommand(ssh_session session, string my_stdin, string* my_stdout);
    bool readRemoteTextFile(ssh_session session, string path, string* textfile_content, string* error_desc);
    void parseQStatForRunningJobs(string& input, map<string, string>& joblist);
    void parseSQueueForRunningJobs(string& input, map<string, string>& joblist);
    bool doCallback(CURL *curl, string url, WSDHPCWorkState state, bool ispackage);

    void* runWorkerItemEntry(WSScheduler::WSSchedulerWorkItem* witem);
    void* runBackgroundWorker();

    static void* workerEntry(void* context) {
        WSScheduler::WSSchedulerWorkItem* witem = (WSScheduler::WSSchedulerWorkItem *) context;
        return witem->scheduler->runWorkerItemEntry(witem);
    }

    static void* backgroundWorkerEntry(void* context) {
        WSScheduler* owner = (WSScheduler*) context;
        return owner->runBackgroundWorker();
    }
};

struct WSHPCWork {
    HPCJobType jobtype;
    int jobid;
    int hpcjobid;
    int hpcid;
    int packageid;
    int userid;
    string clientip;
    string packagepath;
    string packagefoldername;
    string programpath;
    string processparameters;
    string outputlist;
    string callbackurl;
    string outputpackageurl;
    string inputfolder;
    string outputdirname;
    string outputfilename;
    HPCProgramType programtype;
    int nodes;
    int ppn;
    string walltime;
    int optional;
    int errorcode;
    bool isworkflow;
    vector<WSWorkflowTask*>* workflow_tasks;
};

#endif	/* WSSCHEDULER_H */

