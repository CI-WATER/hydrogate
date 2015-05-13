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

#ifndef WSHELPER_H
#define	WSHELPER_H

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <sstream>

using namespace std;

class WSHelper {
public:
    static WSHelper* instance();
    const string currentDateTime();
    bool fileExists(string path);
    bool checkFileExtension(string path, string extension);
    string getSafeFileName(string path);
    void replaceStringInPlace(string& subject, const string& search,
            const string& replace, bool forAll);
    string extractStringByRowAndColumn(string& input, int row, int column);

private:
    static WSHelper* instance_;

    WSHelper();
};

#endif	/* WSHELPER_H */

