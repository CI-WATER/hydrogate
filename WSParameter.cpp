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

#include "WSParameter.h"

WSParameter::WSParameter(http::request* request, http::response* response, cache_interface* cache, const json::value* settings) {
    request_ = request;
    response_ = response;
    cache_ = cache;
    settings_ = settings;
}

string WSParameter::getCacheValue(string key) {
    string value;
    cache_->fetch_frame(key, value);
    return value;
}

void WSParameter::setCache(string key, string value) {
    cache_->store_frame(key, value);
}

string WSParameter::getPostValue(string key) {
    return request_->post(key);
}

string WSParameter::getGetValue(string key) {
    return request_->get(key);
}

void WSParameter::sendResponse(string& text) {
    response_->out() << text;
}

void WSParameter::sendResponse(cppcms::json::value& jsondata) {
    response_->set_content_header("application/json");
    response_->out() << jsondata;
}

bool WSParameter::isPost() const {
    if (request_->request_method() == "POST")
        return true;
    return false;
}

bool WSParameter::isGet() const {
    if (request_->request_method() == "GET")
        return true;
    return false;
}

bool WSParameter::isDelete() const {
    if (request_->request_method() == "DELETE")
        return true;
    return false;
}

bool WSParameter::isPut() const {
    if (request_->request_method() == "PUT")
        return true;
    return false;
}

string WSParameter::getVisitorIP() const {
    return request_->remote_addr();
}



