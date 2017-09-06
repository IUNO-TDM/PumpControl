#include "pumpdrivershield.h"
#include "easylogging++.h"

using namespace std;

#ifdef OS_raspbian

PumpDriverShield::PumpDriverShield(){
}

PumpDriverShield::~PumpDriverShield(){
}

bool PumpDriverShield::Init(const char* config_txt, const map<int, PumpDefinition>& pump_definitions) {
    LOG(INFO)<< "Init";
    pump_definitions_ = pump_definitions;

    int ec_initialize = gpioInitialise();
    bool rv = false;

    if(ec_initialize >= 0){
        rv= true;
    }

    return rv;
}

void PumpDriverShield::DeInit(){
    gpioTerminate();
}

int PumpDriverShield::GetPumpCount(){
    return 8;
}

float PumpDriverShield::SetFlow(int pump_number, float flow){
    float rv = 0;
    if(flow < pump_definitions_[pump_number].min_flow) {
        //firmata_->analogWrite(pump_definitions_[pump_number].output, 0);
        rv = 0;
    } else {
        int pwm_value = (flow - pump_definitions_[pump_number].min_flow) / (pump_definitions_[pump_number].max_flow - pump_definitions_[pump_number].min_flow) * 128 + 127;
        //firmata_->analogWrite(pump_definitions_[pump_number].output, pwm_value);
        rv = flow;
    }
    return rv;
}

#else

PumpDriverShield::PumpDriverShield(){
}

PumpDriverShield::~PumpDriverShield(){
}

bool PumpDriverShield::Init(const char* config_txt, const map<int, PumpDefinition>& pump_definitions) {
    return false;
}

void PumpDriverShield::DeInit(){
}

#endif


