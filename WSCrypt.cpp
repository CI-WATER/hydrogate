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

#include "WSCrypt.h"
#include "WSException.h"
#include <crypt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

WSCrypt* WSCrypt::instance_ = NULL;
const string WSCrypt::base64_chars_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~-";

WSCrypt::WSCrypt() {
    tokenexpiredurationinsec_ = 60;// default is 1 minute
}

WSCrypt* WSCrypt::instance() {
    if (!instance_)
        instance_ = new WSCrypt;

    return instance_;
}

const string WSCrypt::hash(const string& passwd, const string& salt) {
    struct crypt_data data;
    data.initialized = 0;
    std::stringstream inputSalt;
    inputSalt << "$6$" << salt;

    string hashed = crypt_r(passwd.c_str(), inputSalt.str().c_str(), &data);
    unsigned found = hashed.find_last_of("$");
    if (found == string::npos)
        throw WSException(WS_EXCEPTION_NOT_FOUND, "hash-001");

    return hashed.substr(found + 1);
}

const string WSCrypt::generateSalt() {
    int randDevice = open("/dev/urandom", O_RDONLY);
    char salt[17];
    int i = 0;
    while (true) {
        unsigned char buf;
        ssize_t result = read(randDevice, &buf, sizeof (unsigned char));
        if (result < 0) {
            throw WSException(WS_EXCEPTION_IO, "generateSalt-001");
        }

        if (isalnum(buf))
            salt[i++] = buf;

        if (i == 16)
            break;
    }

    salt[i] = '\0';
    close(randDevice);
    return salt;
}

void WSCrypt::generateSaltForAES(int* salt1, int* salt2) {
    string salt = generateSalt();
    int count = 0;
    for (int i = 0; i < 4; i++) {
        *(((unsigned char*) salt1) + i) = salt[count++];
        *(((unsigned char*) salt2) + i) = salt[count++];
    }
}

inline bool WSCrypt::isBase64(unsigned char c) {
    return (isalnum(c) || (c == '~') || (c == '-'));
}

string WSCrypt::base64Encode(unsigned char const* buf, unsigned int bufLen) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (bufLen--) {
        char_array_3[i++] = *(buf++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars_[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars_[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

unsigned char* WSCrypt::base64Decode(string const& encoded_string, int* length) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<unsigned char> ret;

    while (in_len-- && (encoded_string[in_] != '=') && isBase64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars_.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars_.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    *length = ret.size();
    unsigned char* arr = new unsigned char[*length];
    std::copy(ret.begin(), ret.end(), arr);
    return arr;
}

int WSCrypt::aesInit(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx,
        EVP_CIPHER_CTX *d_ctx) {
    int i, nrounds = 5;
    unsigned char key[32], iv[32];

    /*
     * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
     * nrounds is the number of times the we hash the material. More rounds are more secure but
     * slower.
     */
    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
    if (i != 32) {
        printf("Key size is %d bits - should be 256 bits\n", i);
        return -1;
    }

    EVP_CIPHER_CTX_init(e_ctx);
    EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(d_ctx);
    EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

    return 0;
}

string WSCrypt::aesEncrypt(EVP_CIPHER_CTX *e, string plain) {
    unsigned char *plaintext = (unsigned char*) plain.c_str();
    int len = plain.size() + 1;
    /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
    int c_len = len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *ciphertext = (unsigned char*) malloc(c_len);

    /* allows reusing of 'e' for multiple encryption cycles */
    EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);

    /* update ciphertext, c_len is filled with the length of ciphertext generated,
     *len is the size of plaintext in bytes */
    EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, len);

    /* update ciphertext with the final remaining bytes */
    EVP_EncryptFinal_ex(e, ciphertext + c_len, &f_len);

    len = c_len + f_len;

    string base64res = base64Encode(ciphertext, len);
    free(ciphertext);
    return base64res;
}

string WSCrypt::aesDecrypt(EVP_CIPHER_CTX *e, string cipher) {
    int len;
    unsigned char *ciphertext = base64Decode(cipher, &len);

    /* because we have padding ON, we must allocate an extra cipher block size of memory */
    int p_len = len, f_len = 0;
    unsigned char *plaintext = (unsigned char*) malloc(p_len + AES_BLOCK_SIZE);

    EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, len);
    EVP_DecryptFinal_ex(e, plaintext + p_len, &f_len);

    len = p_len + f_len;

    string plain((char*) plaintext);
    free(plaintext);
    return plain;
}

string WSCrypt::createToken(const string& username, const string& ipaddress, bool isbound) {
    time_t ctime = time(0);
    time_t etime = ctime + tokenexpiredurationinsec_;
    
    cppcms::json::value my_token;
    my_token["username"] = username;
    my_token["ipaddress"] = ipaddress;
    my_token["issuetime"] = ctime;
    my_token["expiretime"] = etime;
    my_token["isbound"] = (isbound ? 1 : 0);

    std::stringstream tokenstr;
    tokenstr << my_token;
    string raw_token = tokenstr.str();
    
    EVP_CIPHER_CTX en, de;
    unsigned int salt[] = {token_salt1_, token_salt2_};    
    unsigned char * key_data = (unsigned char*) token_key_.c_str();
    int key_data_len = token_key_.size();

    if (aesInit(key_data, key_data_len, (unsigned char *) &salt, &en, &de)) {
        throw WSException(WS_EXCEPTION_CRYPTO, "Failed to initialize AES cipher");
    }
    
    string encrypted_token = aesEncrypt(&en, raw_token);
    
    return encrypted_token;
}

void WSCrypt::decodeToken(const string& token, const string& ipaddress, string* username, bool* isExpired, int* remainingExpireTime) {
    EVP_CIPHER_CTX en, de;
    unsigned int salt[] = {token_salt1_, token_salt2_};    
    unsigned char * key_data = (unsigned char*) token_key_.c_str();
    int key_data_len = token_key_.size();

    if (aesInit(key_data, key_data_len, (unsigned char *) &salt, &en, &de)) {
        throw WSException(WS_EXCEPTION_CRYPTO, "Failed to initialize AES cipher");
    }
    
    string dencrypted_token = aesDecrypt(&de, token);
    
    std::stringstream tokenstr;
    tokenstr << dencrypted_token;
    
    cppcms::json::value my_token;
    bool isok = my_token.load(tokenstr, true);
    
    if (!isok)
        throw WSException(WS_EXCEPTION_BAD_TOKEN, "Failed to parse token");
    
    string uname = my_token.get<string>("username");
    if (uname == "")
        throw WSException(WS_EXCEPTION_BAD_TOKEN, "Failed to parse username in token");
    
    *username = uname;
    
    time_t expiretime = my_token.get<time_t>("expiretime");
    time_t rtime = expiretime - time(0);
    
    if (remainingExpireTime)
        *remainingExpireTime = rtime;
    
    if (isExpired) {
        if (rtime <= 0)
            *isExpired = true;
        else
            *isExpired = false;
    } else {
        if (rtime <= 0)
            throw WSException(WS_EXCEPTION_BAD_TOKEN, "Token is expired");
    }
    
    
    
}

string WSCrypt::encrypt(string& plain_text, string key, int token1, int token2) {
    EVP_CIPHER_CTX en, de;
    unsigned int salt[] = {token1, token2};    
    unsigned char * key_data = (unsigned char*) key.c_str();
    int key_data_len = key.size();

    if (aesInit(key_data, key_data_len, (unsigned char *) &salt, &en, &de)) {
        throw WSException(WS_EXCEPTION_CRYPTO, "Failed to initialize AES cipher");
    }
    
    string encrypted = aesEncrypt(&en, plain_text);
    
    return encrypted;
}

string WSCrypt::decrypt(string& encoded_text, string key, int token1, int token2) {
    EVP_CIPHER_CTX en, de;
    unsigned int salt[] = {token1, token2};    
    unsigned char * key_data = (unsigned char*) key.c_str();
    int key_data_len = key.size();

    if (aesInit(key_data, key_data_len, (unsigned char *) &salt, &en, &de)) {
        throw WSException(WS_EXCEPTION_CRYPTO, "Failed to initialize AES cipher");
    }
    
    string dencrypted = aesDecrypt(&de, encoded_text);
    
    return dencrypted;
}


