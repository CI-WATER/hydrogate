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

#include "WSPageRequestToken.h"
#include "WSCrypt.h"
#include "WSException.h"
#include "WSData.h"
#include "WSLogger.h"

WSPageRequestToken::WSPageRequestToken() {
}

void WSPageRequestToken::process(WSParameter& parameter) {
    string visitor_ip = parameter.getVisitorIP();

    try {
        if (parameter.isPost()) {
            string username = parameter.getPostValue("username"); //mandatory
            string password = parameter.getPostValue("password"); //mandatory
            string isbound = parameter.getPostValue("isipbound"); //optional

            if (username == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter username is not supplied");

            if (password == "")
                throw WSException(WS_EXCEPTION_BAD_PARAM, "Parameter password is not supplied");                

            bool isipbound = false;
            if (isbound == "true" || isbound == "True" || isbound == "TRUE")
                isipbound = true;

            WSDUser user_rec;
            WSData::instance()->getUserByName(username, &user_rec);

            string hashed_password = WSCrypt::instance()->hash(password, user_rec.salt);

            if (hashed_password != user_rec.password)
                throw WSException(WS_EXCEPTION_AUTHENTICATION_FAILED, password + " " + hashed_password);

            string tokenstr = WSCrypt::instance()->createToken(username, visitor_ip, isipbound);

            cppcms::json::value response_json;
            response_json["token"] = tokenstr;
            response_json["username"] = username;
            response_json["lifetime"] = WSCrypt::instance()->getTokenExpireDurationInSec();
            response_json["status"] = "success";

            parameter.sendResponse(response_json);

        } else {
            throw WSException(WS_EXCEPTION_BAD_HTTP_METHOD, "The method can be accessed via HTTP POST");
        }
    } catch (WSException& e) {
        cppcms::json::value response_json;
        response_json["description"] = "The request had bad syntax or was impossible to be satisified.";
        response_json["status"] = "error";

        parameter.sendResponse(response_json);

        WSLogger::instance()->log("WSPageRequestToken", visitor_ip, e);
    }
}

