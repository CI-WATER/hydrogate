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

#ifndef WSEXCEPTION_H
#define	WSEXCEPTION_H

#include <iostream>
#include <exception>
#include <sstream>

using namespace std;

enum ExceptionSource {
    WS_EXCEPTION_NOT_DEFINED,
    WS_EXCEPTION_NULL,
    WS_EXCEPTION_IO,
    WS_EXCEPTION_DATABASE,
    WS_EXCEPTION_NOT_FOUND,
    WS_EXCEPTION_FATAL,
    WS_EXCEPTION_CRYPTO,
    WS_EXCEPTION_BAD_PARAM,
    WS_EXCEPTION_BAD_HTTP_METHOD,
    WS_EXCEPTION_AUTHENTICATION_FAILED,
    WS_EXCEPTION_BAD_TOKEN,
    WS_EXCEPTION_SSH_CONNECTION,
    WS_EXCEPTION_SSH_ERROR,
    WS_EXCEPTION_JSON_PARSE,
    WS_EXCEPTION_IRODS
};

class WSException : public exception {
public:
    WSException(string message);
    WSException(ExceptionSource source, string message);
    WSException(ExceptionSource source);
    virtual ~WSException() throw ();
    virtual string exceptionString() const throw();
    virtual const char* what() throw ();
    
    inline string getMessage() const {
        return message_;
    }
    
    inline ExceptionSource getSource() const {
        return source_;
    }
    
private:
    string message_;
    ExceptionSource source_;
    ostringstream* cnvt_;
};

#endif	/* WSEXCEPTION_H */

