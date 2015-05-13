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

#ifndef WSSSHSESSIONPOOL_H
#define	WSSSHSESSIONPOOL_H

#include <iostream>
#include <string>
#include <list>
#include <pthread.h>
#include "WSSshSession.h"

using namespace std;

class WSSshSessionPool {
public:
    WSSshSessionPool(string username, string password, string host, int maxopenconnections);
    WSSshSessionPool(string username, string password, string host, int port, int maxopenconnections);
    
    virtual ~WSSshSessionPool();
    
    WSSshSession* getConnectionFromPool(string clientip);
    void returnConnectionToPool(WSSshSession* mysession);
    
private:
    string username_;
    string password_;
    string host_;
    int port_;
    int maxopenconnections_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    list<WSSshSession*>* idlepool_;
    list<WSSshSession*>* inusepool_;
};

#endif	/* WSSSHSESSIONPOOL_H */

