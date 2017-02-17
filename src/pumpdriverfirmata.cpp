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

    std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();

    LOG(INFO) << "init with config text" << config_text_ptr;
    // char *serial_port = new char[strlen(config_text_ptr)];
    // strcpy(serial_port,config_text_ptr);
    // firmata_ = firmata_new(serial_port); //init Firmata
    // while(!firmata_->isReady) //Wait until device is up
    // firmata_pull(firmata_);
    
    // for (auto port : ports) {
      // std::cout << port.port << std::endl;

      if (firmata_ != NULL) {
        delete firmata_;
        firmata_ = NULL;
      }

      try {
        serialio_ = new firmata::FirmSerial(config_text_ptr);
        if (serialio_->available()) {
          sleep(3); // Seems necessary on linux
          firmata_ = new firmata::Firmata<firmata::Base, firmata::I2C>(serialio_);
        }
      } 
      catch(firmata::IOException e) {
        std::cout << e.what() << std::endl;
      }
      catch(firmata::NotOpenException e) {
        std::cout << e.what() << std::endl;
      }
      // if (firmata_ != NULL && firmata_->ready()) {
      //   break;
      // }
      
    // }

    firmata_->setSamplingInterval(100);


    for(auto i : pump_to_output_){
      if(pump_is_pwm[i.first]){
        firmata_->pinMode(i.second, MODE_PWM);
        // firmata_pinMode(firmata_, i.second, MODE_PWM);
      } else {
        firmata_->pinMode(i.second, MODE_OUTPUT);
        // firmata_pinMode(firmata_, i.second, MODE_OUTPUT);
      }
      
    }

    this_thread::sleep_for(chrono::seconds(2));
    for(auto i : pump_to_output_){
      if(pump_is_pwm[i.first]){
        firmata_->analogWrite(i.second,0);
        // firmata_analogWrite(firmata_, i.second, 0);
      } else {
        firmata_->digitalWrite(i.second,LOW);
        // firmata_digitalWrite(firmata_, i.second, LOW);
      }
    }
};

void PumpDriverFirmata::DeInit(){
    for(auto i : pump_to_output_){
      if(pump_is_pwm[i.first]){
        firmata_->analogWrite(i.second,0);
        // firmata_analogWrite(firmata_, i.second, 0);
      } else {
        firmata_->digitalWrite(i.second,LOW);
        // firmata_digitalWrite(firmata_, i.second, LOW);
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
      pump_definitions->at(i.first).flow_precision = MAX_FLOW;
    }
  }
};

void PumpDriverFirmata::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << " : "<< flow;
    if(pump_is_pwm[pump_number]){
      if(flow < MIN_FLOW){
        firmata_->analogWrite(pump_to_output_[pump_number], 0);
        // firmata_analogWrite(firmata_, pump_to_output_[pump_number], 0);
      } else {
        int pwm_value = (flow - MIN_FLOW) / (MAX_FLOW - MIN_FLOW) * 128 + 127;
        firmata_->analogWrite(pump_to_output_[pump_number], pwm_value);
        // firmata_analogWrite(firmata_, pump_to_output_[pump_number], pwm_value);
      }
    } else {
      firmata_->digitalWrite(pump_to_output_[pump_number], (flow>0) ? HIGH : LOW);
      // firmata_digitalWrite(firmata_, pump_to_output_[pump_number], (flow>0) ? HIGH : LOW);
    }
    
};
