#include "iodriversimulation.h"
#include "easylogging++.h"
#include <unistd.h>

using namespace std;

IoDriverSimulation::IoDriverSimulation():
        client_(NULL),
        sim_thread_(NULL),
        cabinet_door_switch_(false),
        juice_door_switch_(false),
        start_button_(false),
        start_button_illumination_(false)
{
}

IoDriverSimulation::~IoDriverSimulation(){
}

bool IoDriverSimulation::Init(const char* config_text){
    if(config_text[0]){
        LOG(WARNING)<< "Ignoring config '" << config_text << "'.";
    }
    LOG(INFO)<<"Init of simulation io driver";
    return true;
}

void IoDriverSimulation::DeInit(){
    LOG(INFO)<< "DeInit of simulation io driver";
}

void IoDriverSimulation::RegisterCallbackClient(IoDriverCallback* client){
    if(client_){
        throw runtime_error("duplicate register of callback client");
    }
    client_ = client;

    sim_thread_ = new thread(&IoDriverSimulation::Simulate, this);
}

void IoDriverSimulation::UnregisterCallbackClient(IoDriverCallback* client){
    if(client_ != client){
        throw runtime_error("unregister of callback client that isn't registered");
    }

    if(sim_thread_){
        {
            unique_lock<mutex> lock(exit_mutex_);
            exit_condition_.notify_one();
        }
        sim_thread_->join();
        delete sim_thread_;
        sim_thread_ = NULL;
    }

    client_ = NULL;
}

void IoDriverSimulation::Simulate(){
    LOG(INFO)<< "Simulate thread started";

    client_->NewInputState("cabinet_door_switch", cabinet_door_switch_);
    client_->NewInputState("juice_door_switch", juice_door_switch_);
    client_->NewInputState("start_button", start_button_);

    bool exit_now = false;
    unsigned loops = 0;
    bool old_illumination = start_button_illumination_;
    while(!exit_now){
        {
            unique_lock<mutex> lock(exit_mutex_);
            exit_now = (cv_status::no_timeout == exit_condition_.wait_for(lock, chrono::seconds(5)));
        }
        if(!exit_now){
            bool newval = (loops%10 == 4);
            if(cabinet_door_switch_ != newval){
                cabinet_door_switch_ = newval;
                LOG(INFO)<< "Simulate setting cabinet_door_switch to " << cabinet_door_switch_;
                client_->NewInputState("cabinet_door_switch", cabinet_door_switch_);
            }
            newval = (loops%10 == 6);
            if(juice_door_switch_ != newval){
                juice_door_switch_ = newval;
                LOG(INFO)<< "Simulate setting juice_door_switch to " << juice_door_switch_;
                client_->NewInputState("juice_door_switch", juice_door_switch_);
            }
            newval = start_button_illumination_  && !old_illumination;
            old_illumination = start_button_illumination_;
            if(start_button_ != newval){
                start_button_ = newval;
                LOG(INFO)<< "Simulate setting start_button to " << start_button_;
                client_->NewInputState("start_button", start_button_);
            }
        }
        loops++;
    }
    LOG(INFO)<< "Simulate thread exits";
}

void IoDriverSimulation::GetDesc(vector<IoDescription>& desc) const {
    desc.clear();
    IoDescription d;
    d.name_="cabinet_door_switch";
    d.type_ = IoDescription::INPUT;
    desc.push_back(d);
    d.name_="juice_door_switch";
    d.type_ = IoDescription::INPUT;
    desc.push_back(d);
    d.name_="start_button";
    d.type_ = IoDescription::INPUT;
    desc.push_back(d);
    d.name_="start_button_illumination";
    d.type_ = IoDescription::OUTPUT;
    desc.push_back(d);
}

bool IoDriverSimulation::GetValue(const string& name) const {
    bool r;
    if(name == "cabinet_door_switch"){
        r = cabinet_door_switch_;
    } else if(name == "juice_door_switch") {
        r = juice_door_switch_;
    } else if(name == "start_button") {
        r = start_button_;
    } else if(name == "start_button_illumination") {
        r = start_button_illumination_;
    } else {
        throw out_of_range("tried to get value of non-existent io.");
    }
    return r;
}

void IoDriverSimulation::SetValue(const string& name, bool value) {
    if(name == "start_button_illumination") {
        start_button_illumination_ = value;
    } else {
        throw out_of_range("tried to set value of non-existent output or an existing input.");
    }
}

