#include <pumpdriversimulation.h>
#include <easylogging++.h>

using namespace std;

PumpDriverSimulation::PumpDriverSimulation() {
}

PumpDriverSimulation::~PumpDriverSimulation() {
}

bool PumpDriverSimulation::Init(const char*, const map<int, PumpDefinition>& pump_definitions) {
    LOG(INFO)<< "Init";
    pump_definitions_ = pump_definitions;
    return true;
}

void PumpDriverSimulation::DeInit() {
    LOG(INFO)<< "DeInit";
}

int PumpDriverSimulation::GetPumpCount() {
    return pump_definitions_.size();
}

float PumpDriverSimulation::SetFlow(int pump_number, float flow) {
    LOG(INFO)<< "SetFlow " << pump_number << " : "<< flow;
    return flow;
}
