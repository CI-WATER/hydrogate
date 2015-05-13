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

#include "WSLogger.h"
#include "WSHelper.h"

WSLogger* WSLogger::instance_ = NULL;
         
const char* WSLogger::level_strings_[] = {"FATAL ERROR", "INFORMATION", "WARNING", "ERROR"};

WSLogger::WSLogger() {
    logfile_ = fopen("..//log//hpcgatewaylog.txt", "a+");
}

WSLogger::~WSLogger() {
    fclose(logfile_);
}

WSLogger* WSLogger::instance() {
    if (!instance_)
        instance_ = new WSLogger();

    return instance_;
}

void WSLogger::lock() {
    struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
    
    fcntl(fileno(logfile_), F_SETLKW, &fl);
}

void WSLogger::unlock() {
    struct flock fl = {F_UNLCK, SEEK_SET, 0, 0, 0};

    fcntl(fileno(logfile_), F_SETLKW, &fl);
}

void WSLogger::log(const string message) {
    lock();
    
    ostringstream cnvt;

    cnvt.str("");

    cnvt << "[ " << WSHelper::instance()->currentDateTime() << " ] [ " << level_strings_[WS_LOG_INFORMATION] << " ] : [ " << message << " ]" << endl;
    
    fprintf(logfile_, "%s", cnvt.str().c_str());
    
    fflush(logfile_);    
    
    unlock();
}

void WSLogger::log(ErrorLevel level, const string message) {    
    lock();
    
    ostringstream cnvt;

    cnvt.str("");

    cnvt << "[ " << WSHelper::instance()->currentDateTime() << " ] [ " << level_strings_[level] << " ] : [ " << message << " ]" << endl;
    
    fprintf(logfile_, "%s", cnvt.str().c_str());
    
    fflush(logfile_);
    
    unlock();
}

void WSLogger::log(const string source, const string message) {    
    lock();
    
    ostringstream cnvt;

    cnvt.str("");

    cnvt << "[ " << WSHelper::instance()->currentDateTime() << " ] [ " << level_strings_[WS_LOG_ERROR] << " ] [ " << source << " ] : [ " << message << " ]" << endl;
    
    fprintf(logfile_, "%s", cnvt.str().c_str());
    
    fflush(logfile_);
    
    unlock();
}

void WSLogger::log(const string source, const string visitorip, const WSException& e) {
    lock();
    
    ostringstream cnvt;

    cnvt.str("");

    cnvt << "[ " << WSHelper::instance()->currentDateTime() << " ] [ " << level_strings_[WS_LOG_ERROR] << " ] [ " << source << " ] [ " << visitorip << " ] : " << e.exceptionString() << endl;
    
    fprintf(logfile_, "%s", cnvt.str().c_str());
    
    fflush(logfile_);
    
    unlock();
}

void WSLogger::log(const string source, const WSException& e) {
    lock();
    
    ostringstream cnvt;

    cnvt.str("");

    cnvt << "[ " << WSHelper::instance()->currentDateTime() << " ] [ " << level_strings_[WS_LOG_ERROR] << " ] [ " << source << " ] : " << e.exceptionString() << endl;
    
    fprintf(logfile_, "%s", cnvt.str().c_str());
    
    fflush(logfile_);
    
    unlock();
}

void WSLogger::log(const string source, const string visitorip, const string message) {
    lock();
    
    ostringstream cnvt;

    cnvt.str("");

    cnvt << "[ " << WSHelper::instance()->currentDateTime() << " ] [ " << level_strings_[WS_LOG_ERROR] << " ] [ " << source << " ] [ " << visitorip << " ] : [ " << message << "]" << endl;
    
    fprintf(logfile_, "%s", cnvt.str().c_str());
    
    fflush(logfile_);
    
    unlock();
}