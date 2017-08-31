#include "pumpcontrol.h"
#include "pumpcontrolcallback.h"
#include "cryptohelpers.h"

#include "easylogging++.h"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/buffer.h>

#include <cfloat>

using namespace std;
using namespace nlohmann;

PumpControl::PumpControl(PumpDriverInterface* pump_driver,
        map<int, PumpDriverInterface::PumpDefinition> pump_definitions) :
    pumpdriver_(pump_driver),
    pump_definitions_(pump_definitions) {

    for (auto i : kPumpIngredientsInit) {
        pump_ingredients_bimap_.insert(boost::bimap<int, string>::value_type(i.first, i.second));
    }

    for (auto i : pump_definitions_) {
        FlowLog log;
        log.flow = 0;
        log.start_time = chrono::system_clock::now();
        flow_logs_[i.first] = log;
    }

    timeprogramrunner_ = new TimeProgramRunner(this);
    SetPumpControlState(PUMP_STATE_IDLE);
}

PumpControl::~PumpControl() {
    LOG(DEBUG) << "PumpControl destructor";

    timeprogramrunner_->Shutdown();
    delete timeprogramrunner_;

    LOG(DEBUG) << "PumpControl destructor finished";
}

void PumpControl::RegisterCallbackClient(PumpControlCallback* client) {
    lock_guard<mutex> lock(callback_client_mutex_);
    if( !callback_client_ ){
        callback_client_ = client;
        callback_client_->NewPumpControlState(pumpcontrol_state_);
    }else{
        throw logic_error("Tried to register a client where already one is registered");
    }
}

void PumpControl::UnregisterCallbackClient(PumpControlCallback* client) {
    lock_guard<mutex> lock(callback_client_mutex_);
    if( callback_client_ == client ){
        callback_client_ = NULL;
    }else{
        throw logic_error("Tried to unregister a client that isn't registered");
    }
}

void PumpControl::StartProgram(unsigned long product_id, const string& in) {
    string recipe_json_string;
    DecryptProgram(product_id, in, recipe_json_string);
    json j;
    try {
        j = json::parse(recipe_json_string);
        LOG(DEBUG)<< "Got a valid json string.";
    } catch (logic_error& ex) {
        LOG(ERROR)<< "Got an invalid json string. Reason: '" << ex.what() << "'.";
        throw invalid_argument(ex.what());
    }
    int max_time = CreateTimeProgram(j["recipe"], timeprogram_);
    if (max_time > 0) {
        SetPumpControlState(PUMP_STATE_ACTIVE);
        string recipe_id = j["recipe"]["id"];
        string order_name = "NO ORDER NAME";// j["orderName"];
        LOG(DEBUG)<< "Successfully imported recipe: " << recipe_id << " for order: " << order_name;
        timeprogramrunner_->StartProgram(order_name.c_str(), timeprogram_);
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

            switch(timing) {
                case TIMING_BY_MACHINE:
                case TIMING_ALL_FAST_START_PARALLEL: {
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
                    break;
                }
                case TIMING_INTERPOLATED_FINISH_PARALLEL: {
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
                    break;
                }
                case TIMING_SEQUENTIAL: {
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
                    break;
                }
                default: {
                    throw invalid_argument("Invalid timing mode");
                }
            }

            time += sleep + 1;
        }
    } catch(const exception& ex) {
        LOG(ERROR)<<"Failed to create timeprogram: "<<ex.what();
        time = -1;
        lock_guard<mutex> lock(callback_client_mutex_);
        if(callback_client_){
            callback_client_->Error("TimeProgramParseError", 1, ex.what());
        }
        throw;
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
        lock_guard<mutex> lock(callback_client_mutex_);
        if(callback_client_){
            callback_client_->NewPumpControlState(pumpcontrol_state_);
        }
    } else {
        throw not_in_this_state("State switch not allowed.");
    }
}

void PumpControl::TimeProgramRunnerProgressUpdate(string id, int percent) {
    LOG(DEBUG)<< "TimeProgramRunnerProgressUpdate " << percent << " : " << id;
    lock_guard<mutex> lock(callback_client_mutex_);
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
        lock_guard<mutex> lock(callback_client_mutex_);
        if(callback_client_){
            callback_client_->ProgramEnded(id);
        }
    }
    timeprogram_.clear();
}

void PumpControl::PumpDriverAmountWarning(int pump_number, int amountWarningLimit) {
    string ingredient = pump_ingredients_bimap_.left.at(pump_number);
    LOG(DEBUG)<< "PumpDriverAmountWarning: number:" << pump_number << " ingredient: " << ingredient << " Amount warning level: " << amountWarningLimit;
    lock_guard<mutex> lock(callback_client_mutex_);
    if(callback_client_){
        callback_client_->AmountWarning(pump_number, ingredient, amountWarningLimit);
    }
}

void PumpControl::SetAmountForPump(int pump_number, int amount){
    if(pump_ingredients_bimap_.left.find(pump_number) == pump_ingredients_bimap_.left.end()) {
         throw out_of_range("Pump number given doesn't exist");
    }
    pump_amount_map_[pump_number] = amount;
}

string PumpControl::GetIngredientForPump(int pump_number) const{
    if(pump_ingredients_bimap_.left.find(pump_number) == pump_ingredients_bimap_.left.end()) {
        throw out_of_range("Pump number given doesn't exist");
    }
    return pump_ingredients_bimap_.left.at(pump_number);
}

void PumpControl::SetIngredientForPump(int pump_number, const string& ingredient) {
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

PumpDriverInterface::PumpDefinition PumpControl::GetPumpDefinition(size_t pump_number) const {
    const PumpDriverInterface::PumpDefinition& pump_definition = pump_definitions_.at(pump_number);
    return pump_definition;
}

float PumpControl::SwitchPump(size_t pump_number, bool switch_on) {
    if(pumpcontrol_state_ != PUMP_STATE_SERVICE){
        throw not_in_this_state("not in service state");
    }
    const PumpDriverInterface::PumpDefinition& pump_definition = GetPumpDefinition(pump_number);
    float new_flow = switch_on ? pump_definition.max_flow : 0;
    SetFlow(pump_number, new_flow);
    return new_flow;
}

void PumpControl::EnterServiceMode(){
    if((pumpcontrol_state_ == PUMP_STATE_IDLE)||(pumpcontrol_state_ == PUMP_STATE_SERVICE)){
        SetPumpControlState(PUMP_STATE_SERVICE);
    } else {
        throw not_in_this_state("Can't enter service while not in idle.");
    }
}

void PumpControl::LeaveServiceMode(){
    if((pumpcontrol_state_ == PUMP_STATE_IDLE)||(pumpcontrol_state_ == PUMP_STATE_SERVICE)){
        SetPumpControlState(PUMP_STATE_IDLE);
    } else {
        throw not_in_this_state("Can't leave service while not in service.");
    }
}

void PumpControl::TrackAmounts(int pump_number, float flow)
{
    auto it = pump_amount_map_.find(pump_number);
    auto now = chrono::system_clock::now();
    if(it != pump_amount_map_.end()) {
        auto flowLog = flow_logs_[pump_number];
        float lastFlow = flowLog.flow;
        auto startTime = flowLog.start_time;
        auto duration = chrono::duration_cast<chrono::milliseconds>(now - startTime).count();
        if(lastFlow > 0)
        {
            if(lastFlow < pump_amount_map_[pump_number]) {
                pump_amount_map_[pump_number] = pump_amount_map_[pump_number] - lastFlow * duration;
            } else {
                pump_amount_map_[pump_number] = 0;
            }
            if(pump_amount_map_[pump_number]< warn_level) {
                PumpDriverAmountWarning(pump_number, warn_level);
            }
        }
        // LOG(DEBUG) << "Amount left in " << pump_number << " is " << pump_amount_map_[pump_number] << "ml";
    }
}

void PumpControl::SetFlow(size_t pump_number, float flow){
    float effective_flow = pumpdriver_->SetFlow(pump_number, flow);
    TrackAmounts(pump_number, effective_flow);
}

void PumpControl::SetAllPumpsOff(){
    for (auto i : pump_definitions_) {
        SetFlow(i.first, 0);
    }
}

void PumpControl::DecryptProgram(unsigned long product_id, const string& in, string& out){
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    CryptoBuffer buffer;
    CryptoHelpers::Unbase64(in, buffer);
    CryptoHelpers::CmDecrypt(product_id, buffer);

    CryptoBuffer program;
    DecryptPrivate(buffer, program);
    out = program;

    EVP_cleanup();
    ERR_free_strings();
}








