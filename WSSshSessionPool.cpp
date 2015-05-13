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

#include "WSSshSessionPool.h"
#include "WSException.h"
#include "WSLogger.h"

WSSshSessionPool::WSSshSessionPool(string username, string password, string host, int maxopenconnections) {
    username_ = username;
    password_ = password;
    host_ = host;
    port_ = 22;
    maxopenconnections_ = maxopenconnections;
    idlepool_ = new list<WSSshSession*>();
    inusepool_ = new list<WSSshSession*>();
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
}

WSSshSessionPool::WSSshSessionPool(string username, string password, string host, int port, int maxopenconnections) {
    username_ = username;
    password_ = password;
    host_ = host;
    port_ = port;
    maxopenconnections_ = maxopenconnections;
    idlepool_ = new list<WSSshSession*>();
    inusepool_ = new list<WSSshSession*>();
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
}

WSSshSessionPool::~WSSshSessionPool() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
    delete idlepool_;
    delete inusepool_;
}

WSSshSession* WSSshSessionPool::getConnectionFromPool(string clientip) {
    WSSshSession* mysession = NULL;
    pthread_mutex_lock(&mutex_);
    if (idlepool_->size() == 0) {
        if (inusepool_->size() >= maxopenconnections_) {
            // wait until get available connection
            while (idlepool_->size() == 0)
                pthread_cond_wait(&cond_, &mutex_);

            mysession = idlepool_->front();
            idlepool_->pop_front();
            inusepool_->push_back(mysession);
        } else {
            try {
                mysession = new WSSshSession(host_, username_, password_, port_);
            } catch (WSException& e) {
                pthread_mutex_unlock(&mutex_);
                throw;
            }
            inusepool_->push_back(mysession);
        }
    } else {
        mysession = idlepool_->front();
        idlepool_->pop_front();
        inusepool_->push_back(mysession);
    }

    pthread_mutex_unlock(&mutex_);
    return mysession;
}

void WSSshSessionPool::returnConnectionToPool(WSSshSession* mysession) {
    if (mysession == NULL)
        return;
    
    pthread_mutex_lock(&mutex_);
    inusepool_->remove(mysession);
    idlepool_->push_back(mysession);
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);
}

