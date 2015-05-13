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

#ifndef WSSSHSESSION_H
#define	WSSSHSESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <string>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

class WSSshSession {
public:
    WSSshSession(string host, string username, string password);
    WSSshSession(string host, string username, string password, int port);

    virtual ~WSSshSession();
    
    bool isConnected() const;
    
    ssh_session session_;
    
private:
    string host_;
    string username_;
    unsigned int port_;
    volatile bool isconnected_;
    
    static const int MAX_RETRY_COUNT = 3;
    
    void connect(string password);
    void disconnect();
    int verifyKnownhost(string* err);
    
};

#endif	/* WSSSHSESSION_H */

