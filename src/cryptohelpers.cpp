#include "cryptohelpers.h"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

#include "CodeMeter.h"

#include <string.h>
#include <sstream>
#include <stdexcept>

using namespace std;

void CryptoHelpers::Unbase64(const string& in, CryptoBuffer& out){
    unsigned char* buffer = new unsigned char[in.length()+1];
    memset(buffer, 0, in.length()+1);
    BIO* b64 = BIO_new(BIO_f_base64());
    // unfortunately some versions of openssl erroneously lack const at the first arg of the following call, so work around...
    CryptoBuffer workaround;
    workaround.set(in);
    BIO* bmem = BIO_new_mem_buf(workaround, workaround.size());
    bmem = BIO_push(b64, bmem);

    BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
    int read_rc=BIO_read(bmem, buffer, in.length());
    out.set(buffer, read_rc);

    BIO_free_all(bmem);
    delete[] buffer;
}

string CryptoHelpers::CmErrorCodeAsString()
{
    switch(CmGetLastErrorCode()){
        case CMERROR_NO_ERROR: return "no error";
        case CMERROR_ENTRY_NOT_FOUND: return "entry not found";
        case CMERROR_CRC_VERIFY_FAILED: return "CRC verification failed";
        case CMERROR_KEYSOURCEMISSED: return "key source is missing";
        case CMERROR_KEYSOURCEWRONG: return "key source invalid";
        case CMERROR_INVALID_HANDLE: return "invalid handle";
        default:{
            char acErrText[256];
            CmGetLastErrorText(CM_GLET_ERRORTEXT, acErrText, sizeof(acErrText));
            return acErrText;
        }
    }
}

void CryptoHelpers::CmDecrypt(unsigned long product_id, CryptoBuffer& buffer)
{
    CMULONG ulFirmCode = 6000274;
    CMULONG ulProductCode = product_id;
    CMULONG ulExtType = 0;
    CMULONG ulFeatureCode = 0;

    CMACCESS cmAcc;
    memset(&cmAcc, 0, sizeof(cmAcc));
    cmAcc.mflCtrl = CM_ACCESS_NOUSERLIMIT;
    cmAcc.mulFirmCode = ulFirmCode;
    cmAcc.mulProductCode = ulProductCode;
    cmAcc.mulFeatureCode = ulFeatureCode;
    HCMSysEntry hcmEntry = CmAccess(CM_ACCESS_LOCAL , &cmAcc);
    if(!hcmEntry){
        stringstream ss;
        ss << "CmAccess failed: '" << CmErrorCodeAsString() << "'.";
        throw invalid_argument(ss.str());
    }

    CMCRYPT2 hcmCrypt;
    memset(&hcmCrypt, 0, sizeof(hcmCrypt));
    hcmCrypt.mcmBaseCrypt.mflCtrl = CM_CRYPT_FIRMKEY | CM_CRYPT_AES ; //| CM_CRYPT_CHKCRC;
    hcmCrypt.mcmBaseCrypt.mulEncryptionCodeOptions = (1 & CM_CRYPT_UCDELTAMASK) | CM_CRYPT_UCCHECK | CM_CRYPT_ATCHECK | CM_CRYPT_ETCHECK | CM_CRYPT_SAUNLIMITED;
    hcmCrypt.mcmBaseCrypt.mulKeyExtType = ulExtType;
    hcmCrypt.mcmBaseCrypt.mulFeatureCode = ulFeatureCode;
    hcmCrypt.mcmBaseCrypt.mulCrc = 0;
    if(!CmCrypt2(hcmEntry, CM_CRYPT_AES_DEC_CBC, &hcmCrypt, buffer, buffer.size())){
        stringstream ss;
        ss << "CmAccess failed: '" << CmErrorCodeAsString() << "'.";
        CmRelease(hcmEntry);
        throw invalid_argument(ss.str());
    }

    CmRelease(hcmEntry);
}

void CryptoHelpers::RsaDecrypt(const char* rsa_private_key, const CryptoBuffer& in, size_t expected_size, CryptoBuffer& out){

    // unfortunately some versions of openssl erroneously lack const at the first arg of the following call, so work around...
    CryptoBuffer workaround;
    workaround.set(rsa_private_key);
    BIO* keybio = BIO_new_mem_buf(workaround, -1);
    if (!keybio){
        printf( "Failed to create key BIO");
    }
    RSA* rsa = NULL;
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);

    CryptoBuffer buffer(in.size());
    int result = RSA_private_decrypt(in.size(), in, buffer, rsa, RSA_PKCS1_PADDING);
    if(result != static_cast<int>(expected_size)){
        printf( "Unexpected size of %d bytes", result);
    }

    out.set(buffer, expected_size);
}

void CryptoHelpers::AesDecrypt(const CryptoBuffer& aes_key, const CryptoBuffer& iv, CryptoBuffer& buffer){
    AES_KEY dec_key;
    AES_set_decrypt_key(aes_key, aes_key.size()*8, &dec_key);
    CryptoBuffer out(buffer.size());
    CryptoBuffer ivnonconst;
    ivnonconst.set(iv, iv.size());
    AES_cbc_encrypt(buffer, out, buffer.size(), &dec_key, ivnonconst, AES_DECRYPT);
    // remove PKCS#7 padding. Last Byte contains the number of bytes used for padding
    buffer.set(out, out.size() - out[out.size()-1]);
}

