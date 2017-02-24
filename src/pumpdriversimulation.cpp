#include <pumpdriversimulation.h>
#include <easylogging++.h>

PumpDriverSimulation::PumpDriverSimulation(){
    
};

PumpDriverSimulation::~PumpDriverSimulation(){
    
};

void PumpDriverSimulation::Init(const char *config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions){
  LOG(INFO) << "init with config text" << config_text_ptr;
  pump_definitions_ = pump_definitions;
};

void PumpDriverSimulation::DeInit(){
  LOG(INFO) << "deInit";
};
// std::map<int,PumpDriverInterface::PumpDefinition> GetPumps(){
//   return pump_definitions_;
// }

int PumpDriverSimulation::GetPumpCount(){
  return pump_definitions_.size();
}
// void PumpDriverSimulation::GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions){
//   if(pump_definitions->size() > 0){
//     pump_definitions->clear();
//   }
//   for(auto i : pump_definitions_){
//     (*pump_definitions)[i.first] = i.second;
//   }
// };
 
void PumpDriverSimulation::SetPump(int pump_number, float flow){
    LOG(INFO) << "setPump " << pump_number << " : "<< flow;
};
