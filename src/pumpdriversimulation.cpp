#include <pumpdriversimulation.h>
#include <easylogging++.h>

using namespace std;

PumpDriverSimulation::PumpDriverSimulation() {
}

PumpDriverSimulation::~PumpDriverSimulation() {
}

bool PumpDriverSimulation::Init(const char *config_text_ptr,
        std::map<int, PumpDriverInterface::PumpDefinition> pump_definitions) {
    LOG(INFO)<< "init with config text" << config_text_ptr;
    pump_definitions_ = pump_definitions;
    return true;
}

void PumpDriverSimulation::DeInit() {
    LOG(INFO)<< "deInit";
}

int PumpDriverSimulation::GetPumpCount() {
    return pump_definitions_.size();
}

float PumpDriverSimulation::SetFlow(int pump_number, float flow) {
    LOG(INFO)<< "setPump " << pump_number << " : "<< flow;
    return flow;
}
