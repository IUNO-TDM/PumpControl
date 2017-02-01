#include <pumpdriverfirmata.h>
#include <easylogging++.h>
#include <thread>
#include <chrono> 
using namespace std;
PumpDriverFirmata::PumpDriverFirmata(){

};

PumpDriverFirmata::~PumpDriverFirmata(){

};

void PumpDriverFirmata::init(const char *configTextPtr){
    LOG(INFO) << "init with config text" << configTextPtr;

    mFirmata = firmata_new((char*)"/dev/tty.usbserial-A104WO1O"[0]); //init Firmata
    while(!mFirmata->isReady) //Wait until device is up
    firmata_pull(mFirmata);

    for(auto i : mPumpToOutput){
    firmata_pinMode(mFirmata, i.second, MODE_OUTPUT);

    }

    this_thread::sleep_for(chrono::seconds(2));
    for(auto i : mPumpToOutput){
    firmata_digitalWrite(mFirmata, i.second, LOW);

    }
};

void PumpDriverFirmata::deInit(){
    for(auto i : mPumpToOutput){
      firmata_digitalWrite(mFirmata, i.second, LOW);
    }
};

void PumpDriverFirmata::getPumps(std::map<int, PumpDefinition>* pumpDefinitions){
  if(pumpDefinitions->size() > 0){
    pumpDefinitions->clear();
  }
  for(auto i : mPumpToOutput){
    (*pumpDefinitions)[i.first] = PumpDefinition();
    pumpDefinitions->at(i.first).minFlow = FLOW;
    pumpDefinitions->at(i.first).maxFlow = FLOW;
    pumpDefinitions->at(i.first).flowPrecision = 0;
    
  }
};

void PumpDriverFirmata::setPump(int pumpNumber, float flow){
    LOG(INFO) << "setPump " << pumpNumber << flow;
    firmata_digitalWrite(mFirmata,mPumpToOutput[pumpNumber], (flow>0) ? HIGH : LOW);
};
