#include <pumpdriversimulation.h>
#include <easylogging++.h>

using namespace std;
PumpDriverSimulation::PumpDriverSimulation(){
    
};

PumpDriverSimulation::~PumpDriverSimulation(){
    
};

bool PumpDriverSimulation::Init(const char *config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions, PumpDriverCallbackClient* callbackClient){
  LOG(INFO) << "init with config text" << config_text_ptr;
  pump_definitions_ = pump_definitions;
  callback_client_ = callbackClient;
  for(auto i: pump_definitions_){
    FlowLog log;
    log.flow = 0;
    log.start_time = chrono::system_clock::now();
    flow_logs_[i.first] = log;
  }

  return true;
};

void PumpDriverSimulation::DeInit(){
  LOG(INFO) << "deInit";
};


int PumpDriverSimulation::GetPumpCount(){
  return pump_definitions_.size();
}

 
void PumpDriverSimulation::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << " : "<< flow;
    auto it = pump_amount_map_.find(pump_number);
    auto now = chrono::system_clock::now();
    if(it != pump_amount_map_.end()){
      auto flowLog = flow_logs_[pump_number];
      float lastFlow = flowLog.flow;
      auto startTime = flowLog.start_time;
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
      if(lastFlow > 0)
      {
        if(lastFlow * duration < pump_amount_map_[pump_number]){
          pump_amount_map_[pump_number] = pump_amount_map_[pump_number] - lastFlow * duration;
        }else{
          pump_amount_map_[pump_number] = 0;
        }
        if(pump_amount_map_[pump_number]< warn_level){
          callback_client_->PumpDriverAmountWarning(pump_number,pump_amount_map_[pump_number]);
        }
      }
      // LOG(DEBUG) << "Amount left in " << pump_number << " is " << pump_amount_map_[pump_number] << "ml";
    }
    PumpDriverSimulation::FlowLog log;
    log.flow = flow;
    log.start_time =now;
    flow_logs_[pump_number] = log;
};


void PumpDriverSimulation::SetAmountForPump(int pump_number, int amount){
    pump_amount_map_[pump_number] = amount;
}
