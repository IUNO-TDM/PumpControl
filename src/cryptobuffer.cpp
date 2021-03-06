#include "cryptobuffer.h"

#include <string.h>

using namespace std;

CryptoBuffer::CryptoBuffer(): buffer_(NULL), size_(0){
}

CryptoBuffer::CryptoBuffer(size_t size){
    buffer_ = new unsigned char[size+1]; // +1 .. space for forced zero termination
    size_= size;
    memset(buffer_, 0, size_+1);
}

CryptoBuffer::CryptoBuffer(const CryptoBuffer& other): buffer_(NULL), size_(0){
	set(other.buffer_, other.size_);
}

CryptoBuffer::~CryptoBuffer(){
    clear();
}

CryptoBuffer& CryptoBuffer::operator = (const CryptoBuffer& other){
	set(other.buffer_, other.size_);
	return *this;
}

void CryptoBuffer::clear() {
    if(size_){
        memset(buffer_, 0, size_+1);
        delete[] buffer_;
        buffer_ = NULL;
        size_ = 0;
    }
}

void CryptoBuffer::set(const unsigned char* buffer, size_t size){
    if(size_){
        memset(buffer_, 0, size_+1);
        delete[] buffer_;
        buffer_ = NULL;
        size_ = 0;
    }

    if(size){
        buffer_ = new unsigned char[size+1]; // +1 .. space for forced zero termination
        size_= size;
        memcpy(buffer_, buffer, size_);
        buffer_[size_] = 0;
    }
}

void CryptoBuffer::set(const string& in){
    set(reinterpret_cast<const unsigned char*>(in.c_str()), in.length());
}
