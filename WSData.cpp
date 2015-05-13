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

#include <string.h>

#include "WSData.h"

WSData* WSData::instance_ = NULL;

WSData::WSData() {
    conn_ = NULL;
    conninfo_ = "";
}

WSData::~WSData() {
    if (conn_ != NULL) {
        PQfinish(conn_);
        conn_ = NULL;
    }
}

WSData* WSData::instance() {
    if (!instance_)
        instance_ = new WSData;

    return instance_;
}

void WSData::setConnectionString(string conninfo) {
    conninfo_ = conninfo;
}

string WSData::getConnectionString() const {
    return conninfo_;
}

string WSData::getCurrentTimestamp() {
    time_t currTime;
    struct tm *localTime;
    currTime = time(NULL);

    localTime = localtime(&currTime);
    char current_timestamp[25];

    sprintf(current_timestamp, "%d-%d-%d %d:%d:%d", 1900 + localTime->tm_year, localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    string now(current_timestamp);

    return now;
}

void WSData::checkConnection(string conninfo) {
    if (conn_ == NULL) {
        conn_ = PQconnectdb(conninfo.c_str());

        // Check to see that the backend connection was successfully made
        if (PQstatus(conn_) != CONNECTION_OK) {
            PQfinish(conn_);
            conn_ = NULL;
            throw WSException(WS_EXCEPTION_DATABASE, conninfo + " Failed to connect to the database");
        }
    }
}

void WSData::getUserByName(string user_name, WSDUser* udata) {
    checkConnection(conninfo_);

    char* e_user_name = PQescapeLiteral(conn_, user_name.c_str(), user_name.length());

    std::string sSQL;
    sSQL.append("SELECT user_id, name, password, salt, created, description FROM users WHERE name = ");
    sSQL.append(e_user_name);
    sSQL.append(" AND enabled = 'TRUE'");

    PGresult *res = PQexec(conn_, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        string temp = e_user_name;
        string err = string(PQerrorMessage(conn_));
        PQfreemem(e_user_name);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the requested user data in the database. User: " + temp + " Err: " + err);
    }

    PQfreemem(e_user_name);

    if (udata) {
        int nTuples = PQntuples(res);

        if (nTuples != 1) {
            PQclear(res);
            throw WSException(WS_EXCEPTION_DATABASE, "Failed to retrieve one user data in the database");
        }
        udata->user_id = atoi(PQgetvalue(res, 0, 0));
        udata->name = PQgetvalue(res, 0, 1);
        udata->password = PQgetvalue(res, 0, 2);
        udata->salt = PQgetvalue(res, 0, 3);
        udata->created = PQgetvalue(res, 0, 4);
        udata->description = PQgetvalue(res, 0, 5);
    }

    PQclear(res);
}

bool WSData::isProgramInstalledOnHPC(int hpc_id, string program_name, int* program_id, int* type, string* install_path) {
    checkConnection(conninfo_);

    char hpcidbuffer [15];
    sprintf(hpcidbuffer, "%d", hpc_id);

    char* e_program_name = PQescapeLiteral(conn_, program_name.c_str(), program_name.length());

    std::string sSQL;
    sSQL.append("SELECT programs.program_id, programs.type, hpc_programs.install_path FROM programs INNER JOIN hpc_programs ON programs.program_id = hpc_programs.program_id WHERE programs.enabled = 'TRUE' AND hpc_programs.enabled = 'TRUE' AND hpc_programs.hpc_id = '");
    sSQL.append(hpcidbuffer);
    sSQL.append("' AND programs.name = ");
    sSQL.append(e_program_name);

    PGresult *res = PQexec(conn_, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        PQfreemem(e_program_name);
        return false;
    }

    PQfreemem(e_program_name);

    if (program_id) {
        *program_id = atoi(PQgetvalue(res, 0, 0));
    }

    if (type) {
        *type = atoi(PQgetvalue(res, 0, 1));
    }

    if (install_path) {
        *install_path = PQgetvalue(res, 0, 2);
    }

    PQclear(res);

    return true;
}

void WSData::getProgramInfoByName(string program_name, int* type, string* description, string* version, map<string, WSDProcessParameter*>& optional, map<string, WSDProcessParameter*>& nonoptional, list<WSDAuthor*>& authors) {
    checkConnection(conninfo_);

    char* e_program_name = PQescapeLiteral(conn_, program_name.c_str(), program_name.length());

    std::string sSQL;
    sSQL.append("SELECT program_id, type, description, version FROM programs WHERE name = ");
    sSQL.append(e_program_name);
    sSQL.append(" AND enabled = 'TRUE'");

    PGresult *res = PQexec(conn_, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        PQfreemem(e_program_name);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not found program record in the database. Program name: " + program_name);
    }

    PQfreemem(e_program_name);

    int program_id = atoi(PQgetvalue(res, 0, 0));

    if (type) {
        *type = atoi(PQgetvalue(res, 0, 1));
    }

    if (description) {
        *description = string(PQgetvalue(res, 0, 2));
    }

    if (version) {
        *version = string(PQgetvalue(res, 0, 3));
    }

    PQclear(res);

    sSQL = "";

    char programidbuffer [15];
    sprintf(programidbuffer, "%d", program_id);

    sSQL.append("SELECT name, optional, systemcontrolled, datatype, option, default_value, max, min, description FROM process_parameters WHERE program_id = '");
    sSQL.append(programidbuffer);
    sSQL.append("'");

    res = PQexec(conn_, sSQL.c_str());

    rettype = PQresultStatus(res);

    int nTuples, i;

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
    } else {

        nTuples = PQntuples(res);

        for (i = 0; i < nTuples; i++) {
            string name = string(PQgetvalue(res, i, 0));
            char optionalf = *((char*) PQgetvalue(res, i, 1));
            char systemcontrolledf = *((char*) PQgetvalue(res, i, 2));
            WSDProcessParameterType datatype = (WSDProcessParameterType) atoi(PQgetvalue(res, i, 3));
            string option = string(PQgetvalue(res, i, 4));
            string default_value = string(PQgetvalue(res, i, 5));
            double max = atof(PQgetvalue(res, i, 6));
            double min = atof(PQgetvalue(res, i, 7));
            string description = string(PQgetvalue(res, i, 8));

            WSDProcessParameter* pparam = new WSDProcessParameter(datatype, option, default_value, max, min, description);

            if (systemcontrolledf == 'f') {
                if (optionalf == 't') {
                    optional[name] = pparam;
                } else {
                    nonoptional[name] = pparam;
                }
            }
        }

        PQclear(res);
    }

    sSQL = "";

    sSQL.append("SELECT name, surname, email, phone, affiliation FROM authors INNER JOIN program_authors ON authors.author_id = program_authors.author_id WHERE program_authors.program_id = '");
    sSQL.append(programidbuffer);
    sSQL.append("'");

    res = PQexec(conn_, sSQL.c_str());

    rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
    } else {

        nTuples = PQntuples(res);

        for (i = 0; i < nTuples; i++) {
            WSDAuthor* author = new WSDAuthor();
            author->name = string(PQgetvalue(res, i, 0));
            author->surname = string(PQgetvalue(res, i, 1));
            author->email = string(PQgetvalue(res, i, 2));
            author->phone = string(PQgetvalue(res, i, 3));
            author->affiliation = string(PQgetvalue(res, i, 4));

            authors.push_back(author);
        }

        PQclear(res);
    }
}

void WSData::getPackageFolderInformationByID(bool ismultithreaded, int package_id, WSDHPCWorkState* state, int* hpc_id, string* folder) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char packageidbuffer [15];
    sprintf(packageidbuffer, "%d", package_id);

    std::string sSQL;
    sSQL.append("SELECT hpc_id, state, folder FROM packages WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not found package record in the database. PackageID: " + string(packageidbuffer));
    }

    if (hpc_id) {
        *hpc_id = atoi(PQgetvalue(res, 0, 0));
    }

    if (state) {
        *state = (WSDHPCWorkState) atoi(PQgetvalue(res, 0, 1));
    }

    if (folder) {
        *folder = string(PQgetvalue(res, 0, 2));
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::deleteJobByID(bool ismultithreaded, int jobid) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", jobid);
    
    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobStateDeleted);
    string strstate = string(statebuffer);

    std::string sSQL;
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(strstate);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the job in the database. JobID: " + string(jobidbuffer));
    }

    PQclear(res);

    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getPackageByID(bool ismultithreaded, int packageid, WSDPackage& packagedata) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char packageidbuffer[15];
    sprintf(packageidbuffer, "%d", packageid);

    std::string sSQL;
    sSQL.append("SELECT hpc_id, user_id, state, folder, callbackurl FROM packages WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the package data in the database. PackageID: " + string(packageidbuffer));
    }

    packagedata.hpc_id = atoi(PQgetvalue(res, 0, 0));
    packagedata.user_id = atoi(PQgetvalue(res, 0, 1));
    packagedata.state = atoi(PQgetvalue(res, 0, 2));
    packagedata.folder = string(PQgetvalue(res, 0, 3));
    packagedata.callbackurl = string(PQgetvalue(res, 0, 4));

    PQclear(res);

    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getJobByID(bool ismultithreaded, int jobid, WSDJob& jobdata) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", jobid);

    std::string sSQL;
    sSQL.append("SELECT user_id, hpc_id, package_id, state, hpc_job_id, hpc_job_desc, callbackurl FROM jobs WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the job data in the database. JobID: " + string(jobidbuffer));
    }

    jobdata.job_id = jobid;
    jobdata.user_id = atoi(PQgetvalue(res, 0, 0));
    jobdata.hpc_id = atoi(PQgetvalue(res, 0, 1));
    jobdata.package_id = atoi(PQgetvalue(res, 0, 2));
    jobdata.state = atoi(PQgetvalue(res, 0, 3));
    jobdata.hpc_job_id = atoi(PQgetvalue(res, 0, 4));
    jobdata.hpc_job_desc = string(PQgetvalue(res, 0, 5));
    jobdata.callbackurl = string(PQgetvalue(res, 0, 6));

    PQclear(res);

    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::deletePackageByID(bool ismultithreaded, int packageid) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char packageidbuffer [15];
    sprintf(packageidbuffer, "%d", packageid);

    // check package status
    std::string sSQL;
    sSQL.append("SELECT state FROM packages WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");
    
    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the package in the database. PackageID: " + string(packageidbuffer));
    }

    WSDHPCWorkState packagestate = (WSDHPCWorkState) atoi(PQgetvalue(res, 0, 0));
    
    PQclear(res);
    
    if (packagestate == WSDPackageDeleted) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Package is already deleted. PackageID: " + string(packageidbuffer));
    }
    
     if (packagestate == WSDPackageStateDone) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Package transfer is not completed. PackageID: " + string(packageidbuffer));
    }
    
    // check statuses of the jobs associated with the package
    sSQL = "";
    sSQL.append("SELECT state FROM jobs WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");
    
    res = PQexec(myconn, sSQL.c_str());

    rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the jobs in the database with PackageID: " + string(packageidbuffer));
    }
    
    int nTuples = PQntuples(res);
    int i;

    for (i = 0; i < nTuples; i++) {
        WSDHPCWorkState jobstate = (WSDHPCWorkState) atoi(PQgetvalue(res, i, 0));
        
        if (jobstate == WSDJobStateDeleted || jobstate == WSDJobOutputStateTransferDone)
            continue;
        
         PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "One of the job with PackageID: " + string(packageidbuffer) + " is not completed. State: " + string(packageidbuffer));
    }
    
    PQclear(res);
    
    // set all jobs as deleted
    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobStateDeleted);
    string strstate = string(statebuffer);
    
    sSQL = "";
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(strstate);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    res = PQexec(myconn, sSQL.c_str());

    rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not set the job as deleted in the database. PackageID: " + string(packageidbuffer));
    }

    PQclear(res);
    
    // set package as deleted
    sprintf(statebuffer, "%d", (int) WSDPackageDeleted);
    strstate = string(statebuffer);
    
    sSQL = "";
    sSQL.append("UPDATE packages SET state = '");
    sSQL.append(strstate);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    res = PQexec(myconn, sSQL.c_str());

    rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the package in the database. PackageID: " + string(packageidbuffer));
    }

    PQclear(res);

    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getHPCTemplateByID(bool ismultithreaded, int hpcid, WSDHPCTemplate* hpc_template) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char hpcidbuffer [15];
    sprintf(hpcidbuffer, "%d", hpcid);

    std::string sSQL;
    sSQL.append("SELECT hpc_id, pbs_script_header, pbs_script_task, pbs_script_tail FROM hpc_templates WHERE hpc_id = '");
    sSQL.append(hpcidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the requested hpc template data in the database. HPCID: " + string(hpcidbuffer));
    }

    if (hpc_template) {
        hpc_template->hpc_id = atoi(PQgetvalue(res, 0, 0));
        hpc_template->pbs_script_header = PQgetvalue(res, 0, 1);
        hpc_template->pbs_script_task = PQgetvalue(res, 0, 2);
        hpc_template->pbs_script_tail = PQgetvalue(res, 0, 3);
    }

    PQclear(res);

    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getHpcCenterByID(bool ismultithreaded, int hpcid, WSDHpcCenter* hpcdata) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char hpcidbuffer [15];
    sprintf(hpcidbuffer, "%d", hpcid);

    std::string sSQL;
    sSQL.append("SELECT hpc_id, name, address, port, account_name, account_password, tsalt1, tsalt2, created, qtype FROM hpc_centers WHERE hpc_id = '");
    sSQL.append(hpcidbuffer);
    sSQL.append("' AND enabled = 'TRUE'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);

        if (myconn != conn_)
            PQfinish(myconn);

        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the requested hpc center data in the database. HPCID: " + string(hpcidbuffer));
    }

    if (hpcdata) {
        int nTuples = PQntuples(res);

        if (nTuples != 1) {
            PQclear(res);
            throw WSException(WS_EXCEPTION_DATABASE, "Failed to retrieve one hpc center data in the database");
        }

        hpcdata->hpc_id = atoi(PQgetvalue(res, 0, 0));
        hpcdata->name = PQgetvalue(res, 0, 1);
        hpcdata->address = PQgetvalue(res, 0, 2);
        hpcdata->port = atoi(PQgetvalue(res, 0, 3));
        hpcdata->account_name = PQgetvalue(res, 0, 4);
        hpcdata->account_password = PQgetvalue(res, 0, 5);
        hpcdata->tsalt1 = atoi(PQgetvalue(res, 0, 6));
        hpcdata->tsalt2 = atoi(PQgetvalue(res, 0, 7));
        hpcdata->created = PQgetvalue(res, 0, 8);
        hpcdata->qtype = atoi(PQgetvalue(res, 0, 9));
    }

    PQclear(res);

    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getHpcCenterByName(string hpc_name, WSDHpcCenter* hpcdata) {
    checkConnection(conninfo_);

    char* e_hpc_name = PQescapeLiteral(conn_, hpc_name.c_str(), hpc_name.length());

    std::string sSQL;
    sSQL.append("SELECT hpc_id, name, address, port, account_name, account_password, tsalt1, tsalt2, created FROM hpc_centers WHERE name = ");
    sSQL.append(e_hpc_name);
    sSQL.append(" AND enabled = 'TRUE'");

    PGresult *res = PQexec(conn_, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        string temp = e_hpc_name;
        PQfreemem(e_hpc_name);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not found the requested hpc center data in the database. HPC: " + temp);
    }

    PQfreemem(e_hpc_name);

    if (hpcdata) {
        int nTuples = PQntuples(res);

        if (nTuples != 1) {
            PQclear(res);
            throw WSException(WS_EXCEPTION_DATABASE, "Failed to retrieve one hpc center data in the database");
        }
        hpcdata->hpc_id = atoi(PQgetvalue(res, 0, 0));
        hpcdata->name = PQgetvalue(res, 0, 1);
        hpcdata->address = PQgetvalue(res, 0, 2);
        hpcdata->port = atoi(PQgetvalue(res, 0, 3));
        hpcdata->account_name = PQgetvalue(res, 0, 4);
        hpcdata->account_password = PQgetvalue(res, 0, 5);
        hpcdata->tsalt1 = atoi(PQgetvalue(res, 0, 6));
        hpcdata->tsalt2 = atoi(PQgetvalue(res, 0, 7));
        hpcdata->created = PQgetvalue(res, 0, 8);
    }

    PQclear(res);
}

bool WSData::hasUserRightOnHPC(int user_id, string hpc_name, int* hpc_id) {
    checkConnection(conninfo_);

    char* e_hpc_name = PQescapeLiteral(conn_, hpc_name.c_str(), hpc_name.length());

    std::string sSQL;
    sSQL.append("SELECT hpc_id FROM hpc_centers WHERE name = ");
    sSQL.append(e_hpc_name);
    sSQL.append(" AND enabled = 'TRUE'");

    PGresult *res = PQexec(conn_, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        PQfreemem(e_hpc_name);
        return false;
    }

    PQfreemem(e_hpc_name);

    int nTuples = PQntuples(res);

    if (nTuples != 1) {
        PQclear(res);
        return false;
    }

    if (hpc_id) {
        *hpc_id = atoi(PQgetvalue(res, 0, 0));
    }

    PQclear(res);

    return true;
}

int WSData::createPackage(bool ismultithreaded, int user_id, int hpc_id, string callback) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char useridbuffer [15];
    sprintf(useridbuffer, "%d", user_id);
    char hpcidbuffer [15];
    sprintf(hpcidbuffer, "%d", hpc_id);

    string current_time = getCurrentTimestamp();
    char* e_callback = PQescapeLiteral(conn_, callback.c_str(), callback.length());
    
    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDPackageStateInQueue);
    string strstate = string(statebuffer);

    std::string sSQL;
    sSQL.append("INSERT INTO packages (hpc_id, user_id, state, folder, stdin, stdout, stderr, created, updated, callbackurl) VALUES ('");
    sSQL.append(hpcidbuffer);
    sSQL.append("', '");
    sSQL.append(useridbuffer);
    sSQL.append("', '");
    sSQL.append(strstate);
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(current_time);
    sSQL.append("', '");
    sSQL.append(current_time);
    sSQL.append("', '");
    sSQL.append(e_callback);
    sSQL.append("') RETURNING package_id");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQfreemem(e_callback);
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not insert package record into the the database.");
    }

    int package_id = atoi(PQgetvalue(res, 0, 0));

    PQfreemem(e_callback);
    PQclear(res);
    
    if (myconn != conn_)
        PQfinish(myconn);

    return package_id;
}

int WSData::createJob(bool ismultithreaded, int user_id, int hpc_id, int package_id, string description, string definition, string callback) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char useridbuffer [15];
    char hpcidbuffer [15];
    char packageidbuffer [15];

    sprintf(useridbuffer, "%d", user_id);
    sprintf(hpcidbuffer, "%d", hpc_id);
    sprintf(packageidbuffer, "%d", package_id);

    char* e_description = PQescapeLiteral(conn_, description.c_str(), description.length());
    char* e_definition = PQescapeLiteral(conn_, definition.c_str(), definition.length());
    char* e_callback = PQescapeLiteral(conn_, callback.c_str(), callback.length());

    string current_time = getCurrentTimestamp();

    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobStateInServiceQueue);
    string strstate = string(statebuffer);
    
    std::string sSQL;
    sSQL.append("INSERT INTO jobs (user_id, hpc_id, package_id, state, hpc_job_id, hpc_job_desc, stdin, stdout, stderr, errcode, description, created, updated, definition, callbackurl) VALUES ('");
    sSQL.append(useridbuffer);
    sSQL.append("', '");
    sSQL.append(hpcidbuffer);
    sSQL.append("', '");
    sSQL.append(packageidbuffer);
    sSQL.append("', '");
    sSQL.append(strstate);
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append(" ");
    sSQL.append("', '");
    sSQL.append("-1");
    sSQL.append("', ");
    sSQL.append(e_description);
    sSQL.append(", '");
    sSQL.append(current_time);
    sSQL.append("', '");
    sSQL.append(current_time);
    sSQL.append("', ");
    sSQL.append(e_definition);
    sSQL.append(", ");
    sSQL.append(e_callback);
    sSQL.append(") RETURNING job_id");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQfreemem(e_description);
        PQfreemem(e_definition);
        PQfreemem(e_callback);
        string err = string(PQresultErrorMessage(res));
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not insert job record into the the database. Error: " + err);
    }

    PQfreemem(e_description);
    PQfreemem(e_definition);
    PQfreemem(e_callback);

    int job_id = atoi(PQgetvalue(res, 0, 0));

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);

    return job_id;
}

void WSData::updatePackageError(bool ismultithreaded, int package_id, string my_stderr, string my_stdin, string my_stdout) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char* e_error = PQescapeLiteral(myconn, my_stderr.c_str(), my_stderr.length());
    char* e_stdin = PQescapeLiteral(myconn, my_stdin.c_str(), my_stdin.length());
    char* e_stdout = PQescapeLiteral(myconn, my_stdout.c_str(), my_stdout.length());

    char packageidbuffer [15];
    sprintf(packageidbuffer, "%d", package_id);

    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDPackageStateError);
    string strstate = string(statebuffer);
    
    std::string sSQL;
    sSQL.append("UPDATE packages SET state = '");
    sSQL.append(strstate);
    sSQL.append("', stderr = ");
    sSQL.append(e_error);
    sSQL.append(", stdin = ");
    sSQL.append(e_stdin);
    sSQL.append(", stdout = ");
    sSQL.append(e_stdout);
    sSQL.append(", updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);
        PQfreemem(e_error);
        PQfreemem(e_stdin);
        PQfreemem(e_stdout);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "PCK1: Could not update the package record in the the database.");
    }

    PQfreemem(e_error);
    PQfreemem(e_stdin);
    PQfreemem(e_stdout);

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::updatePackageState(bool ismultithreaded, int package_id, WSDHPCWorkState state) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char packageidbuffer [15];
    char statebuffer [15];

    sprintf(packageidbuffer, "%d", package_id);
    sprintf(statebuffer, "%d", (int) state);

    std::string sSQL;
    sSQL.append("UPDATE packages SET state = '");
    sSQL.append(statebuffer);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not update the package record in the the database.");
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::setJobState(bool ismultithreaded, int job_id, WSDHPCWorkState state, bool* isstatechanged) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", job_id);
    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) state);

    WSDHPCWorkState laststate;

    if (isstatechanged) {
        std::string sSQL;
        sSQL.append("SELECT state FROM jobs WHERE job_id ='");
        sSQL.append(jobidbuffer);
        sSQL.append("'");

        PGresult *res = PQexec(myconn, sSQL.c_str());

        ExecStatusType rettype = PQresultStatus(res);

        if (rettype != PGRES_TUPLES_OK) {
            PQclear(res);
            if (myconn != conn_)
                PQfinish(myconn);
            throw WSException(WS_EXCEPTION_DATABASE, "Could not retrieve job state in the database.");
        }

        laststate = (WSDHPCWorkState) atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
    }

    std::string sSQL;
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(statebuffer);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not update the job state in the the database.");
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);

    if (isstatechanged) {
        if (state == laststate)
            *isstatechanged = false;
        else
            *isstatechanged = true;
    }
}

string WSData::getPackageStatus(bool ismultithreaded, int package_id) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char packageidbuffer [15];

    sprintf(packageidbuffer, "%d", package_id);

    std::string sSQL;
    sSQL.append("SELECT state FROM packages WHERE package_id ='");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not retrieve package state in the database.");
    }

    char* resval = PQgetvalue(res, 0, 0);

    int state = -1;

    if (resval == NULL || strcmp(resval, "") == 0) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_NOT_FOUND, "Could not find the package record.");
    } else {
        state = atoi(resval);
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
    }

    string str_state = "unknown";
    switch (state) {    
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
            str_state = "PackagePreparing";
            break;
        case WSDPackageStateTransferring:
            str_state = "PackageTransferring";
            break;
        case WSDPackageStateUnzipping:
            str_state = "PackageUnzipping";
            break;
        case WSDPackageStateDone:
            str_state = "PackageTransferDone";
            break;
    }

    return str_state;
}

void WSData::setPackageFolder(bool ismultithreaded, int package_id, string folder) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char* e_folder = PQescapeLiteral(myconn, folder.c_str(), folder.length());

    char packageidbuffer [15];
    sprintf(packageidbuffer, "%d", package_id);

    std::string sSQL;
    sSQL.append("UPDATE packages SET folder = ");
    sSQL.append(e_folder);
    sSQL.append(", updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE package_id = '");
    sSQL.append(packageidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);
        PQfreemem(e_folder);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "PCK3: Could not update the package record in the the database.");
    }

    PQfreemem(e_folder);

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::setJobErrorByID(bool ismultithreaded, int job_id, string my_stderr, string my_stdin, string my_stdout) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char* e_stderr = PQescapeLiteral(myconn, my_stderr.c_str(), my_stderr.length());
    char* e_stdin = PQescapeLiteral(myconn, my_stdin.c_str(), my_stdin.length());
    char* e_stdout = PQescapeLiteral(myconn, my_stdout.c_str(), my_stdout.length());

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", job_id);

    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobStateError);
    string strstate = string(statebuffer);
    
    std::string sSQL;
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(strstate);
    sSQL.append("', stdin = ");
    sSQL.append(e_stdin);
    sSQL.append(", stdout = ");
    sSQL.append(e_stdout);
    sSQL.append(", stderr = ");
    sSQL.append(e_stderr);
    sSQL.append(", updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQfreemem(e_stderr);
        PQfreemem(e_stdin);
        PQfreemem(e_stdout);

        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not update the job error in the the database. JOBID: " + string(jobidbuffer));
    }

    PQfreemem(e_stderr);
    PQfreemem(e_stdin);
    PQfreemem(e_stdout);

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::setJobFailedByID(bool ismultithreaded, int job_id, string my_stderr, string my_stdout, int error_code) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char* e_stderr = PQescapeLiteral(myconn, my_stderr.c_str(), my_stderr.length());
    char* e_stdout = PQescapeLiteral(myconn, my_stdout.c_str(), my_stdout.length());

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", job_id);

    char errcodebuffer [15];
    sprintf(errcodebuffer, "%d", error_code);

    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobStateError);
    string strstate = string(statebuffer);
    
    std::string sSQL;
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(strstate);
    sSQL.append("', stderr = ");
    sSQL.append(e_stderr);
    sSQL.append(", stdout = ");
    sSQL.append(e_stdout);
    sSQL.append(", errcode = '");
    sSQL.append(errcodebuffer);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQfreemem(e_stderr);
        PQfreemem(e_stdout);
        
        string errstring = string(PQerrorMessage(myconn));

        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not update the job failed state in the the database. JOBID: " + string(jobidbuffer) + " Error: " + errstring);
    }

    PQfreemem(e_stderr);
    PQfreemem(e_stdout);

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::setJobDoneByID(bool ismultithreaded, int job_id, string my_stderr, string my_stdout, int error_code) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char* e_stderr = PQescapeLiteral(myconn, my_stderr.c_str(), my_stderr.length());
    char* e_stdout = PQescapeLiteral(myconn, my_stdout.c_str(), my_stdout.length());

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", job_id);

    char errcodebuffer [15];
    sprintf(errcodebuffer, "%d", error_code);

    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobOutputStateTransferDone);
    string strstate = string(statebuffer);
    
    std::string sSQL;
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(strstate);
    sSQL.append("', stderr = ");
    sSQL.append(e_stderr);
    sSQL.append(", stdout = ");
    sSQL.append(e_stdout);
    sSQL.append(", errcode = '");
    sSQL.append(errcodebuffer);
    sSQL.append("', updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQfreemem(e_stderr);
        PQfreemem(e_stdout);

        string errstring = string(PQerrorMessage(myconn));
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not update the job failed state in the the database. JOBID: " + string(jobidbuffer) + " Error: " + errstring);
    }

    PQfreemem(e_stderr);
    PQfreemem(e_stdout);

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::setJobSubmittedByID(bool ismultithreaded, int job_id, string hpc_job_id, string hpc_job_desc) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char jobidbuffer [15];
    sprintf(jobidbuffer, "%d", job_id);

    char* e_hpc_job_id = PQescapeLiteral(myconn, hpc_job_id.c_str(), hpc_job_id.length());
    char* e_hpc_job_desc = PQescapeLiteral(myconn, hpc_job_desc.c_str(), hpc_job_desc.length());

    char statebuffer [15];
    sprintf(statebuffer, "%d", (int) WSDJobStateInHPCQueue);
    string strstate = string(statebuffer);
    
    std::string sSQL;
    sSQL.append("UPDATE jobs SET state = '");
    sSQL.append(strstate);
    sSQL.append("', hpc_job_id = ");
    sSQL.append(e_hpc_job_id);
    sSQL.append(", hpc_job_desc = ");
    sSQL.append(e_hpc_job_desc);
    sSQL.append(", updated = '");
    sSQL.append(getCurrentTimestamp());
    sSQL.append("' WHERE job_id = '");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_COMMAND_OK) {
        PQclear(res);
        PQfreemem(e_hpc_job_id);
        PQfreemem(e_hpc_job_desc);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not update the job record in the the database.");
    }

    PQfreemem(e_hpc_job_id);
    PQfreemem(e_hpc_job_desc);

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getProgramParameters(int program_id, map<string, WSDProcessParameter*>& systemcontrolled, map<string, WSDProcessParameter*>& optional, map<string, WSDProcessParameter*>& nonoptional) {
    checkConnection(conninfo_);

    char programidbuffer [15];
    sprintf(programidbuffer, "%d", program_id);

    std::string sSQL;
    sSQL.append("SELECT name, optional, systemcontrolled, datatype, option, default_value, max, min FROM process_parameters WHERE program_id = '");
    sSQL.append(programidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(conn_, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        return;
    }

    int nTuples = PQntuples(res);
    int i;

    for (i = 0; i < nTuples; i++) {
        string name = string(PQgetvalue(res, i, 0));
        char optionalf = *((char*) PQgetvalue(res, i, 1));
        char systemcontrolledf = *((char*) PQgetvalue(res, i, 2));
        WSDProcessParameterType datatype = (WSDProcessParameterType) atoi(PQgetvalue(res, i, 3));
        string option = string(PQgetvalue(res, i, 4));
        string default_value = string(PQgetvalue(res, i, 5));
        double max = atof(PQgetvalue(res, i, 6));
        double min = atof(PQgetvalue(res, i, 7));

        WSDProcessParameter* pparam = new WSDProcessParameter(datatype, option, default_value, max, min);

        if (systemcontrolledf == 't') {
            systemcontrolled[name] = pparam;
        } else {
            if (optionalf == 't') {
                optional[name] = pparam;
            } else {
                nonoptional[name] = pparam;
            }
        }
    }

    PQclear(res);
}

void WSData::freeProcessParameterMap(map<string, WSDProcessParameter*>& pparam) {
    typedef std::map<std::string, WSDProcessParameter*>::iterator it_type;
    auto it_type iterator = pparam.begin();
    while (iterator != pparam.end()) {
        WSDProcessParameter* val = iterator->second;
        delete val;
        pparam.erase(iterator++);
    }
}

string WSData::getJobStatus(bool ismultithreaded, int job_id) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char jobidbuffer [15];

    sprintf(jobidbuffer, "%d", job_id);

    std::string sSQL;
    sSQL.append("SELECT state FROM jobs WHERE job_id ='");
    sSQL.append(jobidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_DATABASE, "Could not retrieve job state in the database.");
    }

    char* resval = PQgetvalue(res, 0, 0);

    int state = -1;

    if (resval == NULL || strcmp(resval, "") == 0) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        throw WSException(WS_EXCEPTION_NOT_FOUND, "Could not find the job record.");
    } else {
        state = atoi(resval);
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
    }

    string stateinstr = "Unidentified";

    switch (state) {
        case WSDJobStateError:
            stateinstr = "JobError";
            break;
        case WSDJobStateInServiceQueue:
            stateinstr = "JobInServiceQueue";
            break;
        case WSDJobStateInHPCQueue:
            stateinstr = "JobInHPCQueue";
            break;
        case WSDJobStateRunning:
            stateinstr = "JobRunning";
            break;
        case WSDJobStateExiting:
            stateinstr = "JobExiting";
            break;
        case WSDJobStateHeld:
            stateinstr = "JobHeld";
            break;
        case WSDJobStateMoved:
            stateinstr = "JobMoved";
            break;
        case WSDJobStateWaiting:
            stateinstr = "JobWaiting";
            break;
        case WSDJobStateSuspended:
            stateinstr = "JobSuspended";
            break;
        case WSDJobStateCompleted:
            stateinstr = "JobCompleted";
            break;
        case WSDJobStateDeleted:
            stateinstr = "JobDeleted";
            break;
        case WSDJobOutputStateTransferInQueue:
            stateinstr = "JobOutputFileTransferInQueue";
            break;
        case WSDJobOutputStateTransferring:
            stateinstr = "JobOutputFileTransferring";
            break;
        case WSDJobOutputStateTransferDone:
            stateinstr = "JobOutputFileTransferDone";
            break;
    }

    return stateinstr;
}

void WSData::getAllSubmittedAndRunningJobsByHPCID(bool ismultithreaded, int hpc_id, list<WSDJobCompact*>& joblist) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char hpcidbuffer [15];
    sprintf(hpcidbuffer, "%d", hpc_id);

    std::string sSQL;
    sSQL.append("SELECT job_id, hpc_id, package_id, state, hpc_job_id, hpc_job_desc, callbackurl FROM jobs WHERE (state = '7' OR state = '8' OR state = '10' OR state = '11' OR state = '12' OR state = '13') AND hpc_id = '");
    sSQL.append(hpcidbuffer);
    sSQL.append("'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        return;
    }

    int nTuples = PQntuples(res);
    int i;

    for (i = 0; i < nTuples; i++) {
        WSDJobCompact* jobdata = new WSDJobCompact;

        jobdata->job_id = atoi(PQgetvalue(res, i, 0));
        jobdata->hpc_id = atoi(PQgetvalue(res, i, 1));
        jobdata->package_id = atoi(PQgetvalue(res, i, 2));
        jobdata->state = (WSDHPCWorkState) atoi(PQgetvalue(res, i, 3));
        jobdata->hpc_job_id = atoi(PQgetvalue(res, i, 4));
        jobdata->hpc_job_desc = PQgetvalue(res, i, 5);
        jobdata->callbackurl = PQgetvalue(res, i, 6);

        joblist.push_back(jobdata);
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getAllProgramNamesByHPCName(bool ismultithreaded, string hpcname, list<string>& programlist) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    char* e_hpcname = PQescapeLiteral(myconn, hpcname.c_str(), hpcname.length());

    std::string sSQL;
    sSQL.append("SELECT programs.name FROM programs INNER JOIN hpc_programs ON programs.program_id = hpc_programs.program_id INNER JOIN hpc_centers ON hpc_programs.hpc_id = hpc_centers.hpc_id WHERE programs.enabled = 'TRUE' AND hpc_programs.enabled = 'TRUE' AND hpc_centers.enabled = 'TRUE' AND hpc_centers.name = ");
    sSQL.append(e_hpcname);

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQfreemem(e_hpcname);
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        return;
    }

    PQfreemem(e_hpcname);

    int nTuples = PQntuples(res);
    int i;

    for (i = 0; i < nTuples; i++) {
        string name = PQgetvalue(res, i, 0);
        programlist.push_back(name);
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getAllHPCNames(bool ismultithreaded, list<string>& hpclist) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    std::string sSQL;
    sSQL.append("SELECT name FROM hpc_centers WHERE enabled = 'TRUE'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        return;
    }

    int nTuples = PQntuples(res);
    int i;

    for (i = 0; i < nTuples; i++) {
        string name = PQgetvalue(res, i, 0);
        hpclist.push_back(name);
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}

void WSData::getAllHPCs(bool ismultithreaded, list<WSDHpcCenter*>& hpclist) {
    PGconn* myconn;
    int is_thread_supported = PQisthreadsafe();

    if (is_thread_supported == 0 && ismultithreaded) {
        myconn = PQconnectdb(conninfo_.c_str());
    } else {
        checkConnection(conninfo_);
        myconn = conn_;
    }

    std::string sSQL;
    sSQL.append("SELECT hpc_id, name, address, port, account_name, account_password, tsalt1, tsalt2, qtype FROM hpc_centers WHERE enabled = 'TRUE'");

    PGresult *res = PQexec(myconn, sSQL.c_str());

    ExecStatusType rettype = PQresultStatus(res);

    if (rettype != PGRES_TUPLES_OK) {
        PQclear(res);
        if (myconn != conn_)
            PQfinish(myconn);
        return;
    }

    int nTuples = PQntuples(res);
    int i;

    for (i = 0; i < nTuples; i++) {
        WSDHpcCenter* hpcdata = new WSDHpcCenter;

        hpcdata->hpc_id = atoi(PQgetvalue(res, i, 0));
        hpcdata->name = PQgetvalue(res, i, 1);
        hpcdata->address = PQgetvalue(res, i, 2);
        hpcdata->port = atoi(PQgetvalue(res, i, 3));
        hpcdata->account_name = PQgetvalue(res, i, 4);
        hpcdata->account_password = PQgetvalue(res, i, 5);
        hpcdata->tsalt1 = atoi(PQgetvalue(res, i, 6));
        hpcdata->tsalt2 = atoi(PQgetvalue(res, i, 7));
        hpcdata->qtype = atoi(PQgetvalue(res, i, 8));

        hpclist.push_back(hpcdata);
    }

    PQclear(res);
    if (myconn != conn_)
        PQfinish(myconn);
}


