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

#include "WSHelper.h"

WSHelper* WSHelper::instance_ = NULL;

WSHelper::WSHelper() {
}

WSHelper* WSHelper::instance() {
    if (!instance_)
        instance_ = new WSHelper;

    return instance_;
}

const string WSHelper::currentDateTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);

    strftime(buf, sizeof (buf), "%Y-%m-%d %X", &tstruct);

    return buf;
}

bool WSHelper::fileExists(string path) {
    ifstream my_file(path.c_str());
    if (my_file.good()) {
        my_file.close();
        return true;
    }
    my_file.close();
    return false;
}

bool WSHelper::checkFileExtension(string path, string extension) {
    size_t where = path.find_last_of('.');

    if (where != string::npos) {
        string mystr = path.substr(where);
        if (mystr == extension)
            return true;
        return false;
    } else {
        return false;
    }
}

string WSHelper::getSafeFileName(string path) {
    size_t where = path.find_last_of('/');

    if (where != string::npos) {
        path = path.substr(where + 1);
    }

    where = path.find_last_of('\\');
    if (where != string::npos) {
        path = path.substr(where + 1);
    }

    return path;
}

void WSHelper::replaceStringInPlace(std::string& subject, const std::string& search,
        const std::string& replace, bool forAll) {
    size_t pos = 0;
    if (forAll) {
        while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    } else {
        if ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }
}

string WSHelper::extractStringByRowAndColumn(string& input, int row, int column) {

    std::istringstream in(input);

    std::string s;

    for (int i = 1; i < row; ++i)
        std::getline(in, s);
    
    std::getline(in, s);

    std::istringstream ss(s);

    std::string nth;
    for (int i = 1; i <= column; ++i)
        ss >> nth;

    return nth;
}
