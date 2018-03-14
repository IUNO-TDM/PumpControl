#include <iodrivergpio.h>
#include <easylogging++.h>
#include "json.hpp"
#include "gpioinithelper.h"
#include <vector>

using namespace std;
using namespace nlohmann;




IoDriverGpio::IoDriverGpio():client_(NULL), gpio_initialized_(false), poll_thread_(NULL){
}

IoDriverGpio::~IoDriverGpio(){
}

bool IoDriverGpio::Init(const char* config_text){
    LOG(INFO)<< "Init with config '" << config_text << "'.";

    if(config_text[0]){
        LOG(DEBUG)<< "Parsing config string for io driver gpio: '" << config_text << "' ...";
        json config_json;
        try {
            config_json = json::parse(string(config_text));
        } catch (logic_error& ex) {
            LOG(ERROR)<< "Got an invalid json string. Reason: '" << ex.what() << "'.";
            throw invalid_argument(ex.what());
        }

        if(!config_json.is_array()){
            string msg("Got an invalid config string. Reason: The string does not contain an array.");
            LOG(ERROR)<< msg;
            throw invalid_argument(msg);
        }

        size_t io_count = config_json.size();
        vector<size_t> pins_used;

        try{
            for(size_t i=0; i<io_count; i++){
                string name=config_json[i]["name"];
                if(gpios_.count(name)){
                    string msg("Got an invalid config string. Reason: The string has multiple ios with the same name.");
                    LOG(ERROR)<< msg;
                    throw invalid_argument(msg);
                }
                GpioDesc gd;
                string pin_type = config_json[i]["type"];
                string polarity = config_json[i]["polarity"];
                gd.type_=ParsePinType(pin_type);
                gd.polarity_=ParsePolarity(polarity);
                gd.pin_=config_json[i]["pin"];
                for(size_t j=0; j<i; j++){
                    if(pins_used[j] == gd.pin_){
                        string msg("Got an invalid config string. Reason: The string has multiple ios with the same pin nr.");
                        LOG(ERROR)<< msg;
                        throw invalid_argument(msg);
                    }
                }
                pins_used.push_back(gd.pin_);
                gpios_[name]=gd;
            }
        }catch (...){
            gpios_.clear();
            throw;
        }
    }

    if(wrapGpioInitialise() < 0){
        throw runtime_error("gpioInitialize failed");
    }

    bool bad=false;
    string last_io_name;
    for(auto i : gpios_){
        last_io_name = i.first;
        try{
            switch(i.second.type_){
                case OUTPUT:
                    bad = bad || gpioSetMode(i.second.pin_, PI_INPUT);
                    bad = bad || gpioSetPullUpDown(i.second.pin_, PI_PUD_OFF);
                    bad = bad || gpioSetMode(i.second.pin_, PI_OUTPUT);
                    Write(i.second, false, i.first);
                    break;
                case INPUT:
                    bad = bad || gpioSetMode(i.second.pin_, PI_INPUT);
                    bad = bad || gpioSetPullUpDown(i.second.pin_, PI_PUD_OFF);
                    i.second.current_value_ = Read(i.second, i.first);
                    break;
                case INPUT_PULLDOWN:
                    bad = bad || gpioSetMode(i.second.pin_, PI_INPUT);
                    bad = bad || gpioSetPullUpDown(i.second.pin_, PI_PUD_DOWN);
                    i.second.current_value_ = Read(i.second, i.first);
                    break;
                case INPUT_PULLUP:
                    bad = bad || gpioSetMode(i.second.pin_, PI_INPUT);
                    bad = bad || gpioSetPullUpDown(i.second.pin_, PI_PUD_UP);
                    i.second.current_value_ = Read(i.second, i.first);
                    break;
            }
        }catch(...){
            bad=true;
        }
    }

    if(bad){
        LOG(ERROR)<<"Setting gpio modes failed for '" << last_io_name << "'.";
        for(auto i : gpios_){
            gpioSetMode(i.second.pin_, PI_INPUT);
            gpioSetPullUpDown(i.second.pin_, PI_PUD_OFF);
        }
        wrapGpioTerminate();
        throw runtime_error("gpio setting modes failed");
    }

    gpio_initialized_ = true;

    return true;
}

void IoDriverGpio::DeInit(){
    if(gpio_initialized_){
        for(auto i : gpios_){
            gpioSetMode(i.second.pin_, PI_INPUT);
            gpioSetPullUpDown(i.second.pin_, PI_PUD_OFF);
        }
        wrapGpioTerminate();
        gpio_initialized_ = false;
    }
    gpios_.clear();
    LOG(INFO)<< "DeInit";
}

void IoDriverGpio::PollLoop(){
    bool exit_now = false;

    for(auto& i : gpios_){
        switch(i.second.type_){
            case OUTPUT:
                break;
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:{
                i.second.current_value_ = Read(i.second, i.first);
                client_->NewInputState(i.first.c_str(), i.second.current_value_);
                break;
            }
        }
    }

    while(!exit_now){
        {
            unique_lock<mutex> lock(exit_mutex_);
            exit_now = (cv_status::no_timeout == exit_condition_.wait_for(lock, chrono::milliseconds(50)));
        }
        if(!exit_now){
            for(auto& i : gpios_){
                switch(i.second.type_){
                    case OUTPUT:
                        break;
                    case INPUT:
                    case INPUT_PULLDOWN:
                    case INPUT_PULLUP:{
                        bool b = Read(i.second, i.first);
                        if(i.second.current_value_ != b){
                            i.second.current_value_ = b;
                            client_->NewInputState(i.first.c_str(), i.second.current_value_);
                        }
                        break;
                    }
                }
            }
        }
    }
}

void IoDriverGpio::RegisterCallbackClient(IoDriverCallback* client){
    if(client_){
        throw runtime_error("duplicate register of callback client");
    }
    client_ = client;
    poll_thread_ = new thread(&IoDriverGpio::PollLoop, this);
}

void IoDriverGpio::UnregisterCallbackClient(IoDriverCallback* client){
    if(client_ != client){
        throw runtime_error("unregister of callback client that isn't registered");
    }

    if(poll_thread_){
        {
            unique_lock<mutex> lock(exit_mutex_);
            exit_condition_.notify_one();
        }
        poll_thread_->join();
        delete poll_thread_;
        poll_thread_=NULL;
    }
    client_ = NULL;
}

void IoDriverGpio::GetDesc(std::vector<IoDescription>& desc) const {
    desc.clear();
    for(auto io : gpios_){
        IoDescription d;
        d.name_=io.first;
        switch(io.second.type_){
            case OUTPUT:
                d.type_ = IoDescription::OUTPUT;
                break;
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:
                d.type_ = IoDescription::OUTPUT;
                break;
        }
        desc.push_back(d);
    }
}

bool IoDriverGpio::GetValue(const string& name) const {
    if(!gpios_.count(name)){
        throw out_of_range("tried to get value of non-existent io.");
    }
    bool r = gpios_.at(name).current_value_;
    return r;
}

void IoDriverGpio::SetValue(const string& name, bool value) {
    if(!gpios_.count(name)){
        throw out_of_range("tried to set value of non-existent output");
    }
    switch(gpios_[name].type_){
        case OUTPUT:
            break;
        case INPUT:
        case INPUT_PULLDOWN:
        case INPUT_PULLUP:
            throw out_of_range("tried to set value of non-existent output");
    }
    Write(gpios_[name], value, name);
}

bool IoDriverGpio::Read(const GpioDesc& gd, const string& name){
    int r=gpioRead(gd.pin_);
    if(r < 0){
        LOG(ERROR)<< "Read from gpio with name '" << name << "' at pin " << gd.pin_ << " failed.";
        throw runtime_error("Read from gpio failed.");
    }
    bool b=false;
    switch (gd.polarity_){
        case ACTIVE_HIGH:
            b = r;
            break;
        case ACTIVE_LOW:
            b = !r;
            break;
    }
    return b;
}

void IoDriverGpio::Write(GpioDesc& gd, bool value, const string& name){
    bool b;
    switch (gd.polarity_){
        case ACTIVE_HIGH:
            b = value;
            break;
        case ACTIVE_LOW:
            b = !value;
            break;
    }
    if(gpioWrite(gd.pin_, b?1:0)){
        LOG(ERROR)<< "Write to gpio with name '" << name << "' at pin " << gd.pin_ << " failed.";
        throw runtime_error("could not set gpio output");
    }
    gd.current_value_ = value;
}

IoDriverGpio::GpioType IoDriverGpio::ParsePinType(const string& str){
    IoDriverGpio::GpioType t;
     if (str == "output"){
        t = IoDriverGpio::OUTPUT;
    } else if (str == "input") {
        t = IoDriverGpio::INPUT;
    } else if (str == "input_pullup") {
        t = IoDriverGpio::INPUT_PULLUP;
    } else if (str == "input_pulldown") {
        t = IoDriverGpio::INPUT_PULLDOWN;
    } else {
        throw invalid_argument("invalid pin type");
    }
    return t;
}

IoDriverGpio::GpioPolarity IoDriverGpio::ParsePolarity(const string& str){
    IoDriverGpio::GpioPolarity p;
    if (str == "active_low"){
       p = IoDriverGpio::ACTIVE_LOW;
   } else if (str == "active_high") {
       p = IoDriverGpio::ACTIVE_HIGH;
   } else {
       throw invalid_argument("invalid polarity value");
   }
   return p;
}


