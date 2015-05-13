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

#include "WSSshSession.h"
#include "WSException.h"

WSSshSession::WSSshSession(string host, string username, string password) {
    host_ = host;
    username_ = username;
    port_ = 22;
    session_ = NULL;
    connect(password);
}

WSSshSession::WSSshSession(string host, string username, string password, int port) {
    host_ = host;
    username_ = username;
    port_ = port;
    session_ = NULL;
    connect(password);
}

WSSshSession::~WSSshSession() {
    disconnect();
}

void WSSshSession::disconnect() {
    if (session_) {
        ssh_disconnect(session_);
        ssh_free(session_);
        session_ = NULL;
    }
}

void WSSshSession::connect(string password) {
    int rc;
    int retry_count = 0;

    session_ = ssh_new();
    if (session_ == NULL) {
        throw WSException(WS_EXCEPTION_SSH_CONNECTION, "Failed to create session object");
    }

    ssh_options_set(session_, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session_, SSH_OPTIONS_USER, username_.c_str());
    ssh_options_set(session_, SSH_OPTIONS_PORT, &port_);

    retry_count = 0;
reconnect1:
    rc = ssh_connect(session_);
    if (rc != SSH_OK) {
        retry_count++;
        if (retry_count == MAX_RETRY_COUNT) {
            ssh_free(session_);
            session_ = NULL;
            string exception_str = "Failed to connect to " + host_ + " with username " + username_;
            throw WSException(WS_EXCEPTION_SSH_CONNECTION, exception_str);
        }
        goto reconnect1;
    }

    string err;
    retry_count = 0;
reconnect2:
    if (verifyKnownhost(&err) < 0) {
        retry_count++;
        if (retry_count == MAX_RETRY_COUNT) {
            ssh_disconnect(session_);
            ssh_free(session_);
            session_ = NULL;
            string exception_str = "Error in verifying " + host_ + " with username " + username_ + ". Description: " + err;
            throw WSException(WS_EXCEPTION_SSH_CONNECTION, exception_str);
        }

        goto reconnect2;
    }

    retry_count = 0;
reconnect3:
    rc = ssh_userauth_password(session_, NULL, password.c_str());
    if (rc != SSH_AUTH_SUCCESS) {
        retry_count++;
        if (retry_count == MAX_RETRY_COUNT) {
            ssh_disconnect(session_);
            ssh_free(session_);
            session_ = NULL;
            string exception_str = "Error in authenticating to " + host_ + ":" + static_cast<ostringstream*>( &(ostringstream() << port_) )->str() + " with username " + username_;
            throw WSException(WS_EXCEPTION_SSH_CONNECTION, exception_str);
        }
        goto reconnect3;
    }
}

int WSSshSession::verifyKnownhost(string* err) {
    return 0;
    /*
    int state, hlen;
    unsigned char *hash = NULL;
    state = ssh_is_server_known(session_);
    hlen = ssh_get_pubkey_hash(session_, &hash);
    if (hlen < 0)
        return -1;

    switch (state) {
        case SSH_SERVER_KNOWN_OK:
            break;
        case SSH_SERVER_KNOWN_CHANGED:
            *err = "SSH_SERVER_KNOWN_CHANGED";
            free(hash);
            return -1;
        case SSH_SERVER_FOUND_OTHER:
            *err = "SSH_SERVER_FOUND_OTHER";
            free(hash);
            return -1;
        case SSH_SERVER_FILE_NOT_FOUND:
        case SSH_SERVER_NOT_KNOWN:
            if (ssh_write_knownhost(session_) < 0) {
                *err = strerror(errno);
                free(hash);
                return -1;
            }
            break;
        case SSH_SERVER_ERROR:
            *err = ssh_get_error(session_);
            free(hash);
            return -1;
    }
    free(hash);
    return 0;*/
}

bool WSSshSession::isConnected() const {
    return (session_ != NULL ? true : false);
}

