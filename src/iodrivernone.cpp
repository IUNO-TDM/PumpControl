#include "iodrivernone.h"
#include "easylogging++.h"
#include <unistd.h>

using namespace std;

IoDriverNone::IoDriverNone():
        client_(NULL)
{
}

IoDriverNone::~IoDriverNone(){
}

bool IoDriverNone::Init(const char* config_text){
    if(config_text[0]){
        LOG(WARNING)<< "Ignoring config '" << config_text << "'.";
    }
    LOG(INFO)<<"Init of io driver none";
    return true;
}

void IoDriverNone::DeInit(){
    LOG(INFO)<< "DeInit of io driver none";
}

void IoDriverNone::RegisterCallbackClient(IoDriverCallback* client){
    if(client_){
        throw runtime_error("duplicate register of callback client");
    }
    client_ = client;
}

void IoDriverNone::UnregisterCallbackClient(IoDriverCallback* client){
    if(client_ != client){
        throw runtime_error("unregister of callback client that isn't registered");
    }
    client_ = NULL;
}

void IoDriverNone::GetDesc(vector<IoDescription>& desc) const {
    desc.clear();
}

bool IoDriverNone::GetValue(const string&) const {
    throw out_of_range("tried to get value of non-existent io.");
}

void IoDriverNone::SetValue(const string&, bool) {
    throw out_of_range("tried to set value of non-existent output or an existing input.");
}

