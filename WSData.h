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

#ifndef WSDATA_H
#define	WSDATA_H

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <stdlib.h>
#include <postgresql/libpq-fe.h>
#include "WSException.h"

using namespace std;

enum WSDHPCWorkState {
    WSDJobStateError = -3,
    WSDPackageStateError = -2,
    WSDHPCWorkUnknownError = -1,
    WSDPackageStateInQueue = 1,
    WSDPackageStateRunning = 2,
    WSDPackageStateTransferring = 3,
    WSDPackageStateUnzipping = 4,
    WSDPackageStateDone = 5,
    WSDJobStateInServiceQueue = 6,
    /*do not change number codes of the states below (see getAllSubmittedAndRunningJobsByHPCID)*/
    WSDJobStateInHPCQueue = 7,
    WSDJobStateRunning = 8,
    WSDJobStateExiting = 9,
    WSDJobStateHeld = 10,
    WSDJobStateMoved = 11,
    WSDJobStateWaiting = 12,
    WSDJobStateSuspended = 13,
    /*end of change*/
    WSDJobStateCompleted = 14,
    WSDJobStateDeleted = 15,
    WSDJobOutputStateTransferInQueue = 16,
    WSDJobOutputStateTransferring = 17,
    WSDJobOutputStateTransferDone = 18,
    WSDPackageDeleted = 19,
    WSDJobStateCompletedInHPC = 20
};

struct WSDUser {
    int user_id;
    string name;
    string password;
    string salt;
    string created;
    string description;
};

struct WSDHpcCenter {
    int hpc_id;
    string name;
    string address;
    int port;
    string account_name;
    string account_password;
    int tsalt1;
    int tsalt2;
    string created;
    int qtype;
};

struct WSDJob {
    int job_id;
    int user_id;
    int hpc_id;
    int package_id;
    int state;
    int hpc_job_id;
    string callbackurl;
    string hpc_job_desc;
    string stdout;
    string stderr;
    string created;
    string updated;
};

struct WSDJobCompact {
    int job_id;
    int hpc_id;
    int package_id;
    WSDHPCWorkState state;
    int hpc_job_id;
    string hpc_job_desc;
    string callbackurl;
};

struct WSDPackage {
    int package_id;
    int hpc_id;
    int user_id;
    int state;
    string folder;
    string callbackurl;
    string stdout;
    string stderr;
    string created;
    string updated;
};

struct WSDHPCTemplate {
    int hpc_id;
    string pbs_script_header;
    string pbs_script_task;
    string pbs_script_tail;
};

struct WSDAuthor {
    string name;
    string surname;
    string email;
    string phone;
    string affiliation;
};

enum WSDProcessParameterType {
    WSDParameterPath = 1,
    WSDParameterString = 2,
    WSDParameterInteger = 3,
    WSDParameterFloat = 4
};

struct WSDProcessParameter {
    WSDProcessParameter(WSDProcessParameterType parameter_type_,
            string option_, string default_value_, double max_, double min_) : parameter_type(parameter_type_), option(option_), default_value(default_value_), max(max_), min(min_) {
    }
    
    WSDProcessParameter(WSDProcessParameterType parameter_type_,
            string option_, string default_value_, double max_, double min_, string description_) : parameter_type(parameter_type_), option(option_), default_value(default_value_), max(max_), min(min_), description(description_)  {
    }
    
    WSDProcessParameterType parameter_type;
    string option;
    string default_value;
    double max;
    double min;
    string description;
};

class WSData {
public:
    WSData();
    virtual ~WSData();
    static WSData* instance();
    void setConnectionString(string conninfo);
    string getConnectionString() const;
    void getUserByName(string user_name, WSDUser* udata);
    void getPackageFolderInformationByID(bool ismultithreaded, int package_id, WSDHPCWorkState* state, int* hpc_id, string* folder);
    void getHpcCenterByName(string hpc_name, WSDHpcCenter* hpcdata);
    void getHpcCenterByID(bool ismultithreaded, int hpcid, WSDHpcCenter* hpcdata);
    int createPackage(bool ismultithreaded, int user_id, int hpc_id, string callback);
    void updatePackageError(bool ismultithreaded, int package_id, string my_stderr, string my_stdin, string my_stdout);
    void updatePackageState(bool ismultithreaded, int package_id, WSDHPCWorkState state);
    void setPackageFolder(bool ismultithreaded, int package_id, string folder);
    string getPackageStatus(bool ismultithreaded, int package_id);
    int createJob(bool ismultithreaded, int user_id, int hpc_id, int package_id, string description, string definition, string callback);
    bool isProgramInstalledOnHPC(int hpc_id, string program_name, int* program_id, int* type, string* install_path);
    void getProgramParameters(int program_id, map<string, WSDProcessParameter*>& systemcontrolled, map<string, WSDProcessParameter*>& optional, map<string, WSDProcessParameter*>& nonoptional);
    void freeProcessParameterMap(map<string, WSDProcessParameter*>& pparam);
    void getHPCTemplateByID(bool ismultithreaded, int hpcid, WSDHPCTemplate* hpc_template);
    void setJobErrorByID(bool ismultithreaded, int job_id, string my_stderr, string my_stdin, string my_stdout);
    void setJobSubmittedByID(bool ismultithreaded, int job_id, string hpc_job_id, string hpc_job_desc);
    string getJobStatus(bool ismultithreaded, int job_id);
    void getAllHPCs(bool ismultithreaded, list<WSDHpcCenter*>& hpclist);
    void getAllHPCNames(bool ismultithreaded, list<string>& hpclist);
    void getAllSubmittedAndRunningJobsByHPCID(bool ismultithreaded, int hpc_id, list<WSDJobCompact*>& joblist);
    void setJobState(bool ismultithreaded, int job_id, WSDHPCWorkState state, bool* isstatechanged);
    bool hasUserRightOnHPC(int user_id, string hpc_name, int* hpc_id);
    void setJobFailedByID(bool ismultithreaded, int job_id, string my_stderr, string my_stdout, int error_code);
    void setJobDoneByID(bool ismultithreaded, int job_id, string my_stderr, string my_stdout, int error_code);
    void getAllProgramNamesByHPCName(bool ismultithreaded, string hpcname, list<string>& programlist);
    void getProgramInfoByName(string program_name, int* type, string* description, string* version, map<string, WSDProcessParameter*>& optional, map<string, WSDProcessParameter*>& nonoptional, list<WSDAuthor*>& authors);
    void deleteJobByID(bool ismultithreaded, int jobid);
    void deletePackageByID(bool ismultithreaded, int packageid);
    void getJobByID(bool ismultithreaded, int jobid, WSDJob& jobdata);
    void getPackageByID(bool ismultithreaded, int packageid, WSDPackage& packagedata);
    
private:
    static WSData* instance_;
    PGconn* conn_;
    string conninfo_;
    void checkConnection(string conninfo);
    string getCurrentTimestamp();
};

#endif	/* WSDATA_H */

