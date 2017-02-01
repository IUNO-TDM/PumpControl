#include <pumpdriversimulation.h>
#include <easylogging++.h>

PumpDriverSimulation::PumpDriverInterface(){
    
};

PumpDriverSimulation::~PumpDriverInterface(){
    
};

void PumpDriverSimulation::init(const char *configTextPtr){
    LOG(INFO) << "init with config text" << configTextPtr;
};

void PumpDriverSimulation::deInit(){
    LOG(INFO) << "deInit";
};

void PumpDriverSimulation::getPumps(std::map<int, PumpDefinition>* pumpDefinitions){
    if(pumpDefinitions->size() > 0){
    pumpDefinitions->clear()
  }
  for(auto i : mPumpToOutput){
    pumpDefinitions->insert(i.first, PumpDefinition());
    pumpDefinitions->at(i.first).minFlow = FLOW;
    pumpDefinitions->at(i.first).maxFlow = FLOW;
    pumpDefinitions->at(i.first).flowPrecision = 0;
  }
};
 
void PumpDriverSimulation::setPump(int pumpNumber, float flow){
    LOG(INFO) << "setPump " << pumpNumber << flow;
};
