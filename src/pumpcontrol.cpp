#include "pumpcontrol.h"
#include "pumpdriversimulation.h"
#include "pumpdriverfirmata.h"
#include "pumpcontrolcallback.h"

#include "easylogging++.h"

#include <cfloat>

using namespace std;
using namespace nlohmann;

PumpControl::PumpControl(string serial_port, bool simulation,
        map<int, PumpDriverInterface::PumpDefinition> pump_definitions) {
    simulation_ = simulation;
    pump_definitions_ = pump_definitions;
    serialport_ = serial_port;

    for (auto i : kPumpIngredientsInit) {
        pump_ingredients_bimap_.insert(boost::bimap<int, string>::value_type(i.first, i.second));
    }

    string config_string;
    if (simulation_) {
        pumpdriver_ = new PumpDriverSimulation();
        LOG(INFO)<< "The simulation mode is on. Firmata not active!";
        config_string = "simulation";

    } else {
        pumpdriver_ = new PumpDriverFirmata();
        config_string = serialport_;
    }

    bool success = pumpdriver_->Init(config_string.c_str(), pump_definitions_, this);
    if (success) {
        timeprogramrunner_ = new TimeProgramRunner(this, pumpdriver_);
        timeprogramrunner_thread_ = thread(&TimeProgramRunner::Run, timeprogramrunner_);
        SetPumpControlState(PUMP_STATE_IDLE);
    } else {
        SetPumpControlState(PUMP_STATE_ERROR);
        LOG(ERROR)<< "Could not initialize the Pump Driver. You can now close the application.";
    }
}

PumpControl::~PumpControl() {
    LOG(DEBUG)<< "PumpControl destructor";

    if(timeprogramrunner_) {
        timeprogramrunner_->Shutdown();
        if(timeprogramrunner_thread_.joinable()) {
            timeprogramrunner_thread_.join();
        }
        delete timeprogramrunner_;
    }
    if(pumpdriver_) {
        delete pumpdriver_;
    }

    LOG(DEBUG) << "PumpControl destructor finished";
}

void PumpControl::RegisterCallbackClient(PumpControlCallback* client) {
    std::lock_guard<std::mutex> lock(callback_client_mutex_);
    if( !callback_client_ ){
        callback_client_ = client;
        callback_client_->NewPumpControlState(pumpcontrol_state_);
    }else{
        throw logic_error("Tried to register a client where already one is registered");
    }
}

void PumpControl::UnregisterCallbackClient(PumpControlCallback* client) {
    std::lock_guard<std::mutex> lock(callback_client_mutex_);
    if( callback_client_ == client ){
        callback_client_ = NULL;
    }else{
        throw logic_error("Tried to unregister a client that isn't registered");
    }
}

void PumpControl::StartProgram(const char* recipe_json_string) {
    SetPumpControlState(PUMP_STATE_ACTIVE);
    json j = json::parse(recipe_json_string);
    int max_time = CreateTimeProgram(j["recipe"], timeprogram_);
    if (max_time > 0) {
        string recipe_id = j["recipe"]["id"];
        string order_name = j["orderName"];
        LOG(DEBUG)<< "Successfully imported recipe: " << recipe_id << " for order: " << order_name;
        timeprogramrunner_->StartProgram(order_name.c_str(), timeprogram_);
    } else {
        SetPumpControlState(PUMP_STATE_IDLE);
    }
}

int PumpControl::CreateTimeProgram(json j, TimeProgramRunner::TimeProgram &timeprogram) {
    LOG(DEBUG)<< "createTimeProgram";
    int time = 0;
    try {
        for(auto line: j["lines"].get<vector<json>>())
        {
            int max_time = time;
            int sleep = line["sleep"].get<int>();
            int timing = line["timing"].get<int>();

            if(timing == 0 || timing == 1) {
                for(auto component: line["components"].get<vector<json>>()) {
                    string ingredient = component["ingredient"].get<string>();
                    int amount = component["amount"].get<int>();
                    int pump_number = pump_ingredients_bimap_.right.at(ingredient);
                    float max_flow = pump_definitions_[pump_number].max_flow;
                    timeprogram[time][pump_number] = max_flow;
                    int end_time = time + amount / max_flow;
                    timeprogram[end_time][pump_number] = 0;

                    if (end_time > max_time) {
                        max_time = end_time;
                    }
                }

                time = max_time;
            } else if (timing == 2) {

                auto component_vector = line["components"].get<vector<json>>();
                map<int, float> min_time_map;
                map<int, float> max_time_map;
                for(auto component: component_vector) {
                    int amount = component["amount"].get<int>();
                    string ingredient = component["ingredient"].get<string>();
                    int pump_number = pump_ingredients_bimap_.right.at(ingredient);
                    float max_flow = pump_definitions_[pump_number].max_flow;
                    float min_flow = pump_definitions_[pump_number].min_flow;

                    min_time_map[pump_number] = ((float)amount) / max_flow;
                    max_time_map[pump_number] = ((float)amount) / min_flow;
                }
                LOG(DEBUG) << "min_time_map:";
                for(auto i : min_time_map) {
                    LOG(DEBUG) << i.first << " : " << i.second;
                }

                LOG(DEBUG) << "max_time_map:";
                for(auto i : max_time_map) {
                    LOG(DEBUG) << i.first << " : " << i.second;
                }

                vector<int> separated_pumps;
                SeparateTooFastIngredients(&separated_pumps,min_time_map, max_time_map);
                LOG(DEBUG) << "separated_pumps:";
                for(auto i : separated_pumps) {
                    LOG(DEBUG) << i;
                }
                float max_duration = min_time_map[GetMaxElement(min_time_map)];
                int end_time = time + (int)max_duration;
                for(auto component: component_vector) {
                    string ingredient = component["ingredient"].get<string>();
                    int amount = component["amount"].get<int>();
                    int pump_number = pump_ingredients_bimap_.right.at(ingredient);

                    if(find(separated_pumps.begin(), separated_pumps.end(),pump_number) != separated_pumps.end() ||
                            pump_definitions_[pump_number].min_flow == pump_definitions_[pump_number].max_flow ||
                            pump_definitions_[pump_number].flow_precision == 0) {
                        float flow = pump_definitions_[pump_number].min_flow;
                        timeprogram[time][pump_number] = flow;
                        int end_time = time + amount / flow;
                        timeprogram[end_time][pump_number] = 0;
                    } else {
                        float flow = ((float)amount) / max_duration;
                        int a = flow / pump_definitions_[pump_number].flow_precision;
                        float difFlow = flow - pump_definitions_[pump_number].flow_precision * (float)a;
                        float difAmount = difFlow * max_duration;
                        LOG(DEBUG) <<"cal Flow: " << flow << "; real Flow: " << pump_definitions_[pump_number].flow_precision * a;
                        LOG(DEBUG) <<"dif Flow: " << difFlow << "; difAmount: " << difAmount;
                        int xtime = end_time;
                        if(difAmount > 0.5) {
                            flow = pump_definitions_[pump_number].flow_precision * (float)(a+1);
                            xtime = (float)amount / flow + time;
                        }
                        timeprogram[time][pump_number] = flow;
                        timeprogram[xtime][pump_number] = 0;
                    }
                }

                time = end_time;
            } else if (timing == 3) {
                for(auto component: line["components"].get<vector<json>>()) {
                    string ingredient = component["ingredient"].get<string>();
                    int amount = component["amount"].get<int>();
                    int pump_number = pump_ingredients_bimap_.right.at(ingredient);
                    float max_flow = pump_definitions_[pump_number].max_flow;
                    timeprogram[time][pump_number] = max_flow;
                    int end_time = time + amount / max_flow;
                    timeprogram[end_time][pump_number] = 0;

                    if (end_time > max_time) {
                        max_time = end_time;
                    }

                    time = max_time;
                }
            }

            time += sleep + 1;
        }
    } catch(const exception& ex) {
        LOG(ERROR)<<"Failed to create timeprogram: "<<ex.what();
        time = -1;
        std::lock_guard<std::mutex> lock(callback_client_mutex_);
        if(callback_client_){
            callback_client_->Error("TimeProgramParseError", 1, ex.what());
        }
    }

    return time;
}

int PumpControl::GetMaxElement(map<int, float> list) {
    float max = FLT_MIN;
    int rv = 0;
    for (auto it : list) {
        if (it.second > max) {
            rv = it.first;
            max = it.second;
        }
    }
    return rv;
}

int PumpControl::GetMinElement(map<int, float> list) {
    float max = FLT_MAX;
    int rv = 0;
    for (auto it : list) {
        if (it.second < max) {
            rv = it.first;
            max = it.second;
        }
    }
    return rv;
}

void PumpControl::SeparateTooFastIngredients(vector<int> *separated_pumps, map<int, float> min_list,
        map<int, float> max_list) {
    int smallest_max_element = GetMinElement(max_list);
    int biggest_min_element = GetMaxElement(min_list);
    LOG(DEBUG)<< "smallest max " << max_list[smallest_max_element] << "biggest min " << min_list[biggest_min_element];
    if (max_list[smallest_max_element] < min_list[biggest_min_element]) {
        separated_pumps->push_back(smallest_max_element);
        min_list.erase(smallest_max_element);
        max_list.erase(smallest_max_element);
        SeparateTooFastIngredients(separated_pumps, min_list, max_list);
    }
}

void PumpControl::SetPumpControlState(PumpControlState state) {
    LOG(DEBUG)<< "SetPumpControlState: " << PumpControlInterface::NameForPumpControlState(state);
    if (pumpcontrol_state_ != state) {
        switch (state) {
            case PUMP_STATE_ACTIVE:
                if (pumpcontrol_state_ == PUMP_STATE_IDLE) {
                    pumpcontrol_state_ = state;
                }
                break;
            case PUMP_STATE_SERVICE:
                if (pumpcontrol_state_ == PUMP_STATE_IDLE) {
                    pumpcontrol_state_ = state;
                }
                break;
            case PUMP_STATE_IDLE:
            case PUMP_STATE_ERROR:
                pumpcontrol_state_ = state;
                break;
            case PUMP_STATE_UNINITIALIZED:
                break;
        }
    }

    if (pumpcontrol_state_ == state) {
        std::lock_guard<std::mutex> lock(callback_client_mutex_);
        if(callback_client_){
            callback_client_->NewPumpControlState(pumpcontrol_state_);
        }
    } else {
        throw logic_error("State switch not allowed.");
    }
}

void PumpControl::TimeProgramRunnerProgressUpdate(string id, int percent) {
    LOG(DEBUG)<< "TimeProgramRunnerProgressUpdate " << percent << " : " << id;
    std::lock_guard<std::mutex> lock(callback_client_mutex_);
    if(callback_client_){
        callback_client_->ProgressUpdate(id, percent);
    }
}

void PumpControl::TimeProgramRunnerStateUpdate(TimeProgramRunnerCallback::State state) {
    LOG(DEBUG)<< "TimeProgramRunnerStateUpdate " << TimeProgramRunnerCallback::NameForState(state);

    switch(state) {
        case TimeProgramRunnerCallback::TIME_PROGRAM_IDLE:
        SetPumpControlState(PUMP_STATE_IDLE);
        break;
        case TimeProgramRunnerCallback::TIME_PROGRAM_ACTIVE:
        SetPumpControlState(PUMP_STATE_ACTIVE);
        break;
        default:
        //do nothing
        break;
    }
}

void PumpControl::TimeProgramRunnerProgramEnded(string id) {
    LOG(DEBUG)<< "TimeProgramRunnerProgramEnded" << id;
    {
        std::lock_guard<std::mutex> lock(callback_client_mutex_);
        if(callback_client_){
            callback_client_->ProgramEnded(id);
        }
    }
    timeprogram_.clear();
}

void PumpControl::PumpDriverAmountWarning(int pump_index, int amountWarningLimit) {
    string ingredient = pump_ingredients_bimap_.left.at(pump_index);
    LOG(DEBUG)<< "PumpDriverAmountWarning: index:" << pump_index << " ingredient: " << ingredient << " Amount warning level: " << amountWarningLimit;
    std::lock_guard<std::mutex> lock(callback_client_mutex_);
    if(callback_client_){
        callback_client_->AmountWarning(pump_index, ingredient, amountWarningLimit);
    }
}

void PumpControl::SetAmountForPump(int pump_number, int amount){
    if(pump_ingredients_bimap_.left.find(pump_number) == pump_ingredients_bimap_.left.end()) {
         throw out_of_range("Pump number given doesn't exist");
    }
    pumpdriver_->SetAmountForPump(pump_number, amount);
}

string PumpControl::GetIngredientForPump(int pump_number) const{
    if(pump_ingredients_bimap_.left.find(pump_number) == pump_ingredients_bimap_.left.end()) {
        throw out_of_range("Pump number given doesn't exist");
    }
    return pump_ingredients_bimap_.left.at(pump_number);
}

void PumpControl::SetIngredientForPump(int pump_number, const std::string& ingredient) {
    auto it = pump_ingredients_bimap_.left.find(pump_number);
    if(it == pump_ingredients_bimap_.left.end())    {
        throw out_of_range("Pump number given doesn't exist");
    }
    pump_ingredients_bimap_.left.replace_data(it, ingredient);
}

void PumpControl::DeleteIngredientForPump(int pump_number){
    if(pump_ingredients_bimap_.left.find(pump_number) == pump_ingredients_bimap_.left.end()) {
        throw out_of_range("Pump number given doesn't exist");
    }
    pump_ingredients_bimap_.left.erase(pump_number);
}

size_t PumpControl::GetNumberOfPumps() const {
    return pump_definitions_.size();
}

PumpDriverInterface::PumpDefinition PumpControl::GetPumpDefinition(size_t pump_index) const {
    const PumpDriverInterface::PumpDefinition& pump_definition = pump_definitions_.at(pump_index);
    return pump_definition;
}

float PumpControl::SwitchPump(size_t pump_index, bool switch_on) {
    if(pumpcontrol_state_ != PUMP_STATE_SERVICE){
        throw logic_error("not in service state");
    }
    const PumpDriverInterface::PumpDefinition& pump_definition = GetPumpDefinition(pump_index);
    float new_flow = switch_on ? pump_definition.max_flow : 0;
    pumpdriver_->SetPump(pump_index, new_flow);
    return new_flow;
}

void PumpControl::EnterServiceMode(){
    if((pumpcontrol_state_ == PUMP_STATE_IDLE)||(pumpcontrol_state_ == PUMP_STATE_SERVICE)){
        SetPumpControlState(PUMP_STATE_SERVICE);
    } else {
        throw logic_error("Can't enter service while not in idle.");
    }
}

void PumpControl::LeaveServiceMode(){
    if((pumpcontrol_state_ == PUMP_STATE_IDLE)||(pumpcontrol_state_ == PUMP_STATE_SERVICE)){
        SetPumpControlState(PUMP_STATE_IDLE);
    } else {
        throw logic_error("Can't leave service while not in service.");
    }
}




