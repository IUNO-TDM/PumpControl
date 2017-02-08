#include <pumpdriverfirmata.h>
#include <easylogging++.h>
#include <thread>
#include <chrono> 
using namespace std;
PumpDriverFirmata::PumpDriverFirmata(){

};

PumpDriverFirmata::~PumpDriverFirmata(){

};

void PumpDriverFirmata::Init(const char *config_text_ptr){
    LOG(INFO) << "init with config text" << config_text_ptr;
    char *serial_port = new char[strlen(config_text_ptr)];
    strcpy(serial_port,config_text_ptr);
    firmata_ = firmata_new(serial_port); //init Firmata
    while(!firmata_->isReady) //Wait until device is up
    firmata_pull(firmata_);

    for(auto i : pump_to_output_){
      if(pump_is_pwm[i.first]){
        firmata_pinMode(firmata_, i.second, MODE_PWM);
      } else {
        firmata_pinMode(firmata_, i.second, MODE_OUTPUT);
      }
      
    }

    this_thread::sleep_for(chrono::seconds(2));
    for(auto i : pump_to_output_){
      if(pump_is_pwm[i.first]){
        firmata_analogWrite(firmata_, i.second, 0);
      } else {
        firmata_digitalWrite(firmata_, i.second, LOW);
      }
    }
};

void PumpDriverFirmata::DeInit(){
    for(auto i : pump_to_output_){
      if(pump_is_pwm[i.first]){
        firmata_analogWrite(firmata_, i.second, 0);
      } else {
        firmata_digitalWrite(firmata_, i.second, LOW);
      }
    }
};

void PumpDriverFirmata::GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions){
  if(pump_definitions->size() > 0){
    pump_definitions->clear();
  }
  for(auto i : pump_to_output_){
    (*pump_definitions)[i.first] = PumpDefinition();
    if(pump_is_pwm[i.first]){
      pump_definitions->at(i.first).min_flow = MIN_FLOW;
      pump_definitions->at(i.first).max_flow = MAX_FLOW;
      pump_definitions->at(i.first).flow_precision = FLOW_PRECISION;
    }else{
      pump_definitions->at(i.first).min_flow = MAX_FLOW;
      pump_definitions->at(i.first).max_flow = MAX_FLOW;
      pump_definitions->at(i.first).flow_precision = 0;
    }
  }
};

void PumpDriverFirmata::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << " : "<< flow;
    if(pump_is_pwm[pump_number]){
      if(flow < MIN_FLOW){
        firmata_analogWrite(firmata_, pump_to_output_[pump_number], 0);
      } else {
        int pwm_value = flow / (MAX_FLOW - MIN_FLOW) * 128 + 127;
        firmata_analogWrite(firmata_, pump_to_output_[pump_number], pwm_value);
      }
    } else {
      firmata_digitalWrite(firmata_, pump_to_output_[pump_number], (flow>0) ? HIGH : LOW);
    }
    
};
