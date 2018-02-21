#include "pumpdrivershield.h"
#include "easylogging++.h"
#include "json.hpp"

#ifdef OS_raspbian
#include <pigpio.h> 
#else
int gpioInitialise(){return -1;}
void gpioTerminate(){}
int gpioSetPWMrange(unsigned, unsigned){return -1;}
int gpioSetPWMfrequency(unsigned, unsigned){return -1;}
int gpioPWM(unsigned, unsigned){return -1;}
int gpioSetMode(unsigned, unsigned){return -1;}
const unsigned PI_OUTPUT = 0;
const unsigned PI_INPUT = 0;
#endif

using namespace std;
using namespace nlohmann;

unsigned PumpDriverShield::pins_[] = {26,6,5,7,11,8,9,25}; // MotorShield V01
const size_t PumpDriverShield::pump_count_ = sizeof(PumpDriverShield::pins_)/sizeof(PumpDriverShield::pins_[0]);

PumpDriverShield::PumpDriverShield(){
}

PumpDriverShield::~PumpDriverShield(){
}

bool PumpDriverShield::Init(const char* config_txt) {
    LOG(INFO)<< "Initializing PumpDriverShield";

    if(config_txt[0]){
    	ParseConfigString(config_txt);
    }

    int ec_initialize = gpioInitialise();
    bool rv = false;

    if(ec_initialize >= 0){
        for(size_t i =0; i<pump_count_; i++){
            unsigned pin = pins_[i];
            gpioSetPWMrange(pin, 1000);
            gpioSetPWMfrequency(pin, 20000);
            gpioPWM(pin, 0);
            gpioSetMode(pin, PI_OUTPUT);
        }
        initialized_ = true;
        LOG(INFO)<< "Library initialized successfully.";
        rv= true;
    }else{
        LOG(ERROR)<< "Library initialization failed.";
    }           

    return rv;
}

void PumpDriverShield::DeInit(){
    if(initialized_){
        for(size_t i =0; i<pump_count_; i++){
            unsigned pin = pins_[i];
            gpioPWM(pin, 0);
            gpioSetMode(pin, PI_INPUT);
        }
        gpioTerminate();
    }
}

void PumpDriverShield::ParseConfigString(const char* config_text){
    LOG(DEBUG)<< "Parsing config string for pump driver shield: '" << config_text << "' ...";
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

    if(pump_count_ != config_json.size()){
    	string msg("Got an invalid config string. Reason: The string does not contain wiring values for 8 pumps.");
        LOG(ERROR)<< msg;
        throw invalid_argument(msg);
    }

    for(size_t i=0; i<pump_count_; i++){
    	if(!config_json[i].is_number_unsigned()){
        	string msg("Got an invalid config string. Reason: The string contains something that isn't a pin number.");
            LOG(ERROR)<< msg;
            throw invalid_argument(msg);
    	}
    	unsigned pin = config_json[i];
    	if((pin < 2) || (27 < pin)){
        	string msg("Got an invalid config string. Reason: The string contains a pin number that is out of range.");
            LOG(ERROR)<< msg;
            throw invalid_argument(msg);
    	}
    }

    for(size_t i=0; i<pump_count_; i++){
        for(size_t j=i+1; j<pump_count_; j++){
        	if(config_json[i] == config_json[j]){
            	string msg("Got an invalid config string. Reason: The string contains a pin that is used multiply.");
                LOG(ERROR)<< msg;
                throw invalid_argument(msg);
        	}
        }
    }

    for(size_t i=0; i<pump_count_; i++){
    	pins_[i]=config_json[i];
    }
    LOG(DEBUG)<< "Parsing config string for pump driver shield done.";
}

int PumpDriverShield::GetPumpCount(){
    return pump_count_;
}

void PumpDriverShield::SetPumpCurrent(int pump_number, float rel_pump_current){
    unsigned pin = GetPinForPump(pump_number);
    unsigned pwm = static_cast<unsigned>(1000.0*rel_pump_current);
    gpioPWM(pin, pwm);
}

unsigned PumpDriverShield::GetPinForPump(size_t pump_number){
    if((pump_number-1) >= pump_count_){
        throw out_of_range("pump number out of range");
    }
    return pins_[pump_number-1];
}

