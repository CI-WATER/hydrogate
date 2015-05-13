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

#ifndef WSPARAMETER_H
#define	WSPARAMETER_H

#include <string>
#include <cppcms/cache_interface.h>
#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <cppcms/json.h>
#include <cppcms/serialization.h>
#include <cppcms/archive_traits.h>

using namespace std;
using namespace cppcms;

class WSParameter {
public:
    WSParameter(http::request* request, http::response* response, cache_interface* cache, const json::value* settings);

    template<typename T>
    T getSetting(string key) {
        return settings_->get<T>(key);
    }

    string getCacheValue(string key);
    void setCache(string key, string value);

    string getPostValue(string key);
    string getGetValue(string key);

    void sendResponse(string& text);
    void sendResponse(cppcms::json::value& jsondata);

    bool isPost() const;
    bool isGet() const;
    bool isPut() const;
    bool isDelete() const;

    string getVisitorIP() const;

private:
    cache_interface* cache_;
    const json::value* settings_;
    http::request* request_;
    http::response* response_;
};

#endif	/* WSPARAMETER_H */

