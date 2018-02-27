#include <iodriversimulation.h>
#include <easylogging++.h>

using namespace std;

IoDriverSimulation::IoDriverSimulation(){
}

IoDriverSimulation::~IoDriverSimulation(){
}

bool IoDriverSimulation::Init(const char* config_text){
    LOG(INFO)<< "Init with config '" << config_text << "'.";;
    return true;
}

void IoDriverSimulation::DeInit(){
    LOG(INFO)<< "DeInit";
}

void IoDriverSimulation::GetDesc(string& desc) const {
    desc="[{\"name\":\"foo\",\"type\":\"input\"}{\"name\":\"bar\",\"type\":\"output\"}]";
    LOG(INFO)<< "GetDesc returning '" << desc << "'";
}
