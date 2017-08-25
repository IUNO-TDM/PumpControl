#ifndef SRC_CRYPTOBUFFER_H_
#define SRC_CRYPTOBUFFER_H_

#include <string>

class CryptoBuffer{
    public:
        CryptoBuffer();

        CryptoBuffer(size_t size);

        ~CryptoBuffer();

        operator const unsigned char* () const {
            return buffer_;
        }

        operator unsigned char* () {
            return buffer_;
        }

        operator std::string () const {
            if(size_){
                buffer_[size_] = 0; // ensure zero termination
                return reinterpret_cast<const char*>(buffer_);
            }else{
                return "";
            }
        }

        size_t size() const {
            return size_;
        }

        void set(const unsigned char* buffer, size_t size);

    private:
        // remark: buffer_ is one byte larger than size_ to support zero termination
        //         as long as size_ is > 0
        unsigned char* buffer_;
        size_t size_;
};

#endif
