#include "pumpdrivershield.h"
#include "easylogging++.h"

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

const unsigned PumpDriverShield::pins_[] = {26,13,6,5,22,27,17,4};
const size_t PumpDriverShield::pump_count_ = sizeof(PumpDriverShield::pins_)/sizeof(PumpDriverShield::pins_[0]);

PumpDriverShield::PumpDriverShield(){
}

PumpDriverShield::~PumpDriverShield(){
}

bool PumpDriverShield::Init(const char* config_txt, const map<int, PumpDefinition>& pump_definitions) {
    LOG(INFO)<< "Initializing PumpDriverShield";
    pump_definitions_ = pump_definitions;

    int ec_initialize = gpioInitialise();
    bool rv = false;

    if(ec_initialize >= 0){
        for(size_t i =0; i<pump_count_; i++){
            unsigned pin = GetPinForPump(i);
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
            unsigned pin = GetPinForPump(i);
            gpioPWM(pin, 0);
            gpioSetMode(pin, PI_INPUT);
        }
        gpioTerminate();
    }
}

int PumpDriverShield::GetPumpCount(){
    return pump_count_;
}

float PumpDriverShield::SetFlow(int pump_number, float flow){
    float rv = 0;
    if(flow < pump_definitions_[pump_number].min_flow) {
        unsigned pin = GetPinForPump(pump_number);
        gpioPWM(pin, 0);
        rv = 0;
    } else {
        unsigned pin = GetPinForPump(pump_number);
        unsigned pwm_value = (flow - pump_definitions_[pump_number].min_flow) / (pump_definitions_[pump_number].max_flow - pump_definitions_[pump_number].min_flow) * 1000;
        gpioPWM(pin, pwm_value);
        rv = flow;
    }
    return rv;
}

unsigned PumpDriverShield::GetPinForPump(size_t pump_number){
    if((pump_number-1) >= pump_count_){
        throw out_of_range("pumpnumber out of range");
    }
    return pins_[pump_number-1];
}

