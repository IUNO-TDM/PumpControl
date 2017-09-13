#include <pumpdriversimulation.h>
#include <easylogging++.h>

using namespace std;

PumpDriverSimulation::PumpDriverSimulation() {
}

PumpDriverSimulation::~PumpDriverSimulation() {
}

bool PumpDriverSimulation::Init(const char*) {
    LOG(INFO)<< "Init";
    return true;
}

void PumpDriverSimulation::DeInit() {
    LOG(INFO)<< "DeInit";
}

int PumpDriverSimulation::GetPumpCount() {
    return 8;
}

void PumpDriverSimulation::SetPumpCurrent(int pump_number, float rel_pump_current) {
    LOG(INFO)<< "SetPumpCurrent " << pump_number << " : "<< rel_pump_current;
}
