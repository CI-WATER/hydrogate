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

#ifndef WSCRYPT_H
#define	WSCRYPT_H

#include <iostream>
#include <sstream>
#include <string>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <ctime>
#include <vector>
#include "WSParameter.h"

using namespace std;

class WSCrypt {
    friend class WSProxy;
    
public:
    static WSCrypt* instance();
    string createToken(const string& username, const string& ipaddress, bool isbound);
    void decodeToken(const string& token, const string& ipaddress, string* username, bool* isExpired=NULL, int* remainingExpireTime=NULL);
    const string hash(const string& passwd, const string& salt);
    string encrypt(string& plain_text, string key, int token1, int token2);
    string decrypt(string& encoded_text, string key, int token1, int token2);
    
    int getTokenExpireDurationInSec(){
        return tokenexpiredurationinsec_;
    }
    
private:
    static WSCrypt* instance_;
    const static string base64_chars_;
    int tokenexpiredurationinsec_;
    string token_key_;
    int token_salt1_;
    int token_salt2_;
    
    void setTokenExpiretionDurationInSec(int value) {
        tokenexpiredurationinsec_ = value;
    }
    
    void setTokenKey(string token_key) {
        token_key_ = token_key;
    }
    
    void setTokenSalt1(int token_salt1) {
        token_salt1_ = token_salt1;
    }
    
    void setTokenSalt2(int token_salt2) {
        token_salt2_ = token_salt2;
    }

    WSCrypt();
    inline bool isBase64(unsigned char c);
    void generateSaltForAES(int* salt1, int* salt2);
    const string generateSalt();
    string base64Encode(unsigned char const* buf, unsigned int bufLen);
    unsigned char* base64Decode(string const& encoded_string, int* length);
    int aesInit(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx,
            EVP_CIPHER_CTX *d_ctx);
    string aesEncrypt(EVP_CIPHER_CTX *e, string plain);
    string aesDecrypt(EVP_CIPHER_CTX *e, string cipher);
};

#endif	/* WSCRYPT_H */

