#include <iodrivergpio.h>
#include <easylogging++.h>
#include "json.hpp"
#include <vector>

#ifdef OS_raspbian
#include <pigpio.h>
#else
#endif

using namespace std;
using namespace nlohmann;

IoDriverGpio::GpioType parse_pin_type(const string& str){
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


IoDriverGpio::IoDriverGpio(){
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
                gd.type_=parse_pin_type(pin_type);
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

    return true;
}

void IoDriverGpio::DeInit(){
    LOG(INFO)<< "DeInit";
    gpios_.clear();
}

void IoDriverGpio::GetDesc(string& desc) const{
    desc = "[";
    bool is_first = true;
    for(auto gpio : gpios_){
        if(!is_first){
            desc.append(",");
        }
        desc.append("{\"name\":\"");
        desc.append(gpio.first);
        desc.append("\",\"type\":\"");
        string type_str;
        switch(gpio.second.type_){
            case INPUT:
            case INPUT_PULLDOWN:
            case INPUT_PULLUP:
                type_str="input";
                break;
            case OUTPUT:
                type_str="output";
                break;
        }
        desc.append(type_str);
        desc.append("\"}");
        is_first=false;
    }
    desc.append("]");
}





