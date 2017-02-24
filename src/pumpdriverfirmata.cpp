#include <pumpdriverfirmata.h>
#include <easylogging++.h>
#include <thread>
#include <chrono> 
using namespace std;
PumpDriverFirmata::PumpDriverFirmata(){

};

PumpDriverFirmata::~PumpDriverFirmata(){

};

void PumpDriverFirmata::Init(const char *config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions){

  std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();
  pump_definitions_ = pump_definitions;

  for(auto i: pump_definitions_){
    if(i.second.max_flow != i.second.min_flow){
      pump_is_pwm_[i.first] = true;
    }else{
      pump_is_pwm_[i.first] = false;
    } 
    
  }

  LOG(INFO) << "init with config text" << config_text_ptr;

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

  firmata_->setSamplingInterval(100);


  for(auto i : pump_definitions_){
    if(pump_is_pwm_[i.first]){
      firmata_->pinMode(i.second.output, MODE_PWM);
      // firmata_pinMode(firmata_, i.second, MODE_PWM);
    } else {
      firmata_->pinMode(i.second.output, MODE_OUTPUT);
      // firmata_pinMode(firmata_, i.second, MODE_OUTPUT);
    }
    
  }

  this_thread::sleep_for(chrono::seconds(2));
  for(auto i : pump_definitions_){
    if(pump_is_pwm_[i.first]){
      firmata_->analogWrite(i.second.output,0);
      // firmata_analogWrite(firmata_, i.second, 0);
    } else {
      firmata_->digitalWrite(i.second.output,LOW);
      // firmata_digitalWrite(firmata_, i.second, LOW);
    }
  }
};

void PumpDriverFirmata::DeInit(){
    for(auto i : pump_definitions_){
      if(pump_is_pwm_[i.first]){
        firmata_->analogWrite(i.second.output,0);
        // firmata_analogWrite(firmata_, i.second, 0);
      } else {
        firmata_->digitalWrite(i.second.output,LOW);
        // firmata_digitalWrite(firmata_, i.second, LOW);
      }
    }
};
int PumpDriverFirmata::GetPumpCount()
{
  return pump_definitions_.size();
}

void PumpDriverFirmata::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << " : "<< flow;
    if(pump_is_pwm_[pump_number]){
      if(flow < pump_definitions_[pump_number].min_flow){
        firmata_->analogWrite(pump_definitions_[pump_number].output, 0);
        // firmata_analogWrite(firmata_, pump_to_output_[pump_number], 0);
      } else {
        int pwm_value = (flow - pump_definitions_[pump_number].min_flow) / (pump_definitions_[pump_number].max_flow - pump_definitions_[pump_number].min_flow) * 128 + 127;
        firmata_->analogWrite(pump_definitions_[pump_number].output, pwm_value);
        // firmata_analogWrite(firmata_, pump_to_output_[pump_number], pwm_value);
      }
    } else {
      firmata_->digitalWrite(pump_definitions_[pump_number].output, (flow>0) ? HIGH : LOW);
      // firmata_digitalWrite(firmata_, pump_to_output_[pump_number], (flow>0) ? HIGH : LOW);
    }
    
};
