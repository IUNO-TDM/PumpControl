#include <pumpdriverfirmata.h>
#include <easylogging++.h>
#include <thread>
using namespace std;
PumpDriverFirmata::PumpDriverFirmata(){

};

PumpDriverFirmata::~PumpDriverFirmata(){

};

bool PumpDriverFirmata::Init(const char *config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions, PumpDriverCallbackClient* callbackClient){
  bool rv = false;
  std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();
  pump_definitions_ = pump_definitions;
  callback_client_ = callbackClient;
  for(auto i: pump_definitions_){
    if(i.second.max_flow != i.second.min_flow){
      pump_is_pwm_[i.first] = true;
    }else{
      pump_is_pwm_[i.first] = false;
    } 

    FlowLog log;
    log.flow = 0;
    log.start_time = chrono::system_clock::now();
    flow_logs_[i.first] = log;
    
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
      firmata_->setSamplingInterval(100);
    }
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
    rv = true;
  } 
  catch(firmata::IOException e) {
    LOG(ERROR)<< e.what();
  }
  catch(firmata::NotOpenException e) {
    LOG(ERROR)<< e.what();
  }
  return rv;
};

void PumpDriverFirmata::DeInit(){
  try{
    for(auto i : pump_definitions_){
      if(pump_is_pwm_[i.first]){
        firmata_->analogWrite(i.second.output,0);
        // firmata_analogWrite(firmata_, i.second, 0);
      } else {
        firmata_->digitalWrite(i.second.output,LOW);
        // firmata_digitalWrite(firmata_, i.second, LOW);
      }
    }
  }catch(const std::exception& ex){
    LOG(ERROR)<<"Exception on DeInit the PumpDriver: "<<ex.what();
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
      } else {
        int pwm_value = (flow - pump_definitions_[pump_number].min_flow) / (pump_definitions_[pump_number].max_flow - pump_definitions_[pump_number].min_flow) * 128 + 127;
        firmata_->analogWrite(pump_definitions_[pump_number].output, pwm_value);
      }
    } else {
      firmata_->digitalWrite(pump_definitions_[pump_number].output, (flow>0) ? HIGH : LOW);
    }
    auto it = pump_amount_map_.find(pump_number);
    auto now = chrono::system_clock::now();
    if(it != pump_amount_map_.end()){
      auto flowLog = flow_logs_[pump_number];
      float lastFlow = flowLog.flow;
      auto startTime = flowLog.start_time;
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
      if(lastFlow > 0)
      {
        if(lastFlow >= pump_amount_map_[pump_number]){
          pump_amount_map_[pump_number] = pump_amount_map_[pump_number] - lastFlow * duration;
        }else{
          pump_amount_map_[pump_number] = 0;
        }
        if(pump_amount_map_[pump_number]< warn_level){
          callback_client_->PumpDriverAmountWarning(pump_number,warn_level);
        }
      }
      // LOG(DEBUG) << "Amount left in " << pump_number << " is " << pump_amount_map_[pump_number] << "ml";
    }
    PumpDriverFirmata::FlowLog log;
    log.flow = flow;
    log.start_time =now;
    flow_logs_[pump_number] = log;
    
};


void PumpDriverFirmata::SetAmountForPump(int pump_number, int amount){
    pump_amount_map_[pump_number] = amount;
}

