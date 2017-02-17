#include <pumpdriversimulation.h>
#include <easylogging++.h>

PumpDriverSimulation::PumpDriverSimulation(){
    
};

PumpDriverSimulation::~PumpDriverSimulation(){
    
};

void PumpDriverSimulation::Init(const char *config_text_ptr){
    LOG(INFO) << "init with config text" << config_text_ptr;
};

void PumpDriverSimulation::DeInit(){
    LOG(INFO) << "deInit";
};

void PumpDriverSimulation::GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions){
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
 
void PumpDriverSimulation::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << " : "<< flow;
};
