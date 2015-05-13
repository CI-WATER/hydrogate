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

#ifndef WSLOGGER_H
#define	WSLOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "WSException.h"

using namespace std;

enum ErrorLevel {
    WS_LOG_FATAL_ERROR = 0,
    WS_LOG_INFORMATION = 1,
    WS_LOG_WARNING = 2,
    WS_LOG_ERROR = 3
};

class WSLogger {
public:
    static WSLogger* instance();
    void log(const string message);
    void log(ErrorLevel level, const string message);
    void log(const string source, const string message);
    void log(const string source, const string visitorip, const WSException& e);
    void log(const string source, const string visitorip, const string message);
    void log(const string source, const WSException& e);
    
    virtual ~WSLogger();
    
private:
    static WSLogger* instance_;
    FILE* logfile_;
    const string logFilePath_;
    
    const static char *level_strings_[];
    
    WSLogger();
    void lock();
    void unlock();
};

#endif	/* WSLOGGER_H */

