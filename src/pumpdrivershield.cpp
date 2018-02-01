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

const unsigned PumpDriverShield::pins_[] = {26,6,5,7,11,8,9,25}; // MotorShield V01
const size_t PumpDriverShield::pump_count_ = sizeof(PumpDriverShield::pins_)/sizeof(PumpDriverShield::pins_[0]);

PumpDriverShield::PumpDriverShield(){
}

PumpDriverShield::~PumpDriverShield(){
}

bool PumpDriverShield::Init(const char* config_txt) {
    LOG(INFO)<< "Initializing PumpDriverShield";

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

