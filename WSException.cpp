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

#include "WSException.h"
#include "WSHelper.h"

WSException::WSException(string message) {
    message_ = message;
    source_ = WS_EXCEPTION_NOT_DEFINED;

    cnvt_ = new ostringstream;
}

WSException::WSException(ExceptionSource source, string message) {
    message_ = message;
    source_ = source;

    cnvt_ = new ostringstream;
}

WSException::WSException(ExceptionSource source) {
    message_ = "";
    source_ = source;

    cnvt_ = new ostringstream;
}

WSException::~WSException() throw () {
    delete cnvt_;
}

const char* WSException::what() throw () {
    message_ = exceptionString();
    return message_.c_str();
}

string WSException::exceptionString() const throw () {
    cnvt_->str("");

    if (source_ != WS_EXCEPTION_NOT_DEFINED) {

        *cnvt_ << "[ ";

        switch (source_) {
            case WS_EXCEPTION_NOT_DEFINED:
                *cnvt_ << "Exception";
                break;
            case WS_EXCEPTION_NULL:
                *cnvt_ << "Null Reference Exception";
                break;
            case WS_EXCEPTION_IO:
                *cnvt_ << "IO Exception";
                break;
            case WS_EXCEPTION_DATABASE:
                *cnvt_ << "Database Exception";
                break;
            case WS_EXCEPTION_NOT_FOUND:
                *cnvt_ << "Not Found Exception";
                break;
            case WS_EXCEPTION_FATAL:
                *cnvt_ << "Fatal Error Exception";
                break;
            case WS_EXCEPTION_CRYPTO:
                *cnvt_ << "Crypto Exception";
                break;
            case WS_EXCEPTION_BAD_PARAM:
                *cnvt_ << "Bad Parameter Exception";
                break;
            case WS_EXCEPTION_BAD_HTTP_METHOD:
                *cnvt_ << "Bad HTTP Method Exception";
                break;
            case WS_EXCEPTION_AUTHENTICATION_FAILED:
                *cnvt_ << "Authentication Exception";
                break;
            case WS_EXCEPTION_BAD_TOKEN:
                *cnvt_ << "Bad Token Exception";
                break;
            case WS_EXCEPTION_SSH_CONNECTION:
                *cnvt_ << "SSH Connection Exception";
                break;
            case WS_EXCEPTION_SSH_ERROR:
                *cnvt_ << "SSH Exception";
                break;
            case WS_EXCEPTION_JSON_PARSE:
                *cnvt_ << "JSON Parse Exception";
                break;
            case WS_EXCEPTION_IRODS:
                *cnvt_ << "IRODS Exception";
                break;  
        }
        
        if (message_ == "") {
            *cnvt_ << " ]";
            return cnvt_->str().c_str();
        }
    }

    if (message_ != "") {
        if (source_ != WS_EXCEPTION_NOT_DEFINED)
            *cnvt_ << " - " << message_ << " ]";
        else
            *cnvt_ << "[ " << message_ << " ]";
    }

    return string(cnvt_->str());
}


