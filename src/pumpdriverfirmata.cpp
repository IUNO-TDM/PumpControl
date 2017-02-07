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

    firmata_ = firmata_new((char*)"/dev/tty.usbserial-A104WO1O"[0]); //init Firmata
    while(!firmata_->isReady) //Wait until device is up
    firmata_pull(firmata_);

    for(auto i : pump_to_output_){
    firmata_pinMode(firmata_, i.second, MODE_OUTPUT);

    }

    this_thread::sleep_for(chrono::seconds(2));
    for(auto i : pump_to_output_){
    firmata_digitalWrite(firmata_, i.second, LOW);

    }
};

void PumpDriverFirmata::DeInit(){
    for(auto i : pump_to_output_){
      firmata_digitalWrite(firmata_, i.second, LOW);
    }
};

void PumpDriverFirmata::GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions){
  if(pump_definitions->size() > 0){
    pump_definitions->clear();
  }
  for(auto i : pump_to_output_){
    (*pump_definitions)[i.first] = PumpDefinition();
    pump_definitions->at(i.first).min_flow = FLOW;
    pump_definitions->at(i.first).max_flow = FLOW;
    pump_definitions->at(i.first).flow_precision = 0;
    
  }
};

void PumpDriverFirmata::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << flow;
    firmata_digitalWrite(firmata_,pump_to_output_[pump_number], (flow>0) ? HIGH : LOW);
};
