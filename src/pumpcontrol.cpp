#include "pumpcontrol.h"
#include "pumpcontrolcallback.h"

#ifndef NO_ENCRYPTION
#	include "cryptohelpers.h"
#endif

#include "easylogging++.h"

#ifndef NO_ENCRYPTION
#	include <openssl/conf.h>
#	include <openssl/evp.h>
#	include <openssl/err.h>
#	include <openssl/buffer.h>
#endif

#include <cfloat>

using namespace std;
using namespace nlohmann;

PumpControl::PumpControl(PumpDriverInterface* pump_driver,
        map<int, PumpDefinition> pump_definitions,
        IoDriverInterface* io_driver) :
    pumpdriver_(pump_driver),
    pump_definitions_(pump_definitions),
    io_driver_(io_driver){

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
    int max_time = 0;
    { // scope for minimized life time of recipe_json
        json recipe_json;
#ifndef NO_ENCRYPTION
        // check for characters that must exist in json and must not exist in base64
        if((in.find("{") == string::npos) && (in.find("}") == string::npos))
        {
        	// this isn't json, so it should be base64 containing an encrypted recipe
        	// scope also minimizes lifetime of recipe_buffer
            CryptoBuffer recipe_buffer;
            DecryptProgram(product_id, in, recipe_buffer);
            try {
                recipe_json = json::parse(string(recipe_buffer.c_str()));
                recipe_buffer.clear(); // clear before logging, logging could be made to block on stdout
                LOG(DEBUG)<< "Got a valid json string.";
            } catch (logic_error& ex) {
                recipe_buffer.clear(); // clear before logging, logging could be made to block on stdout
                LOG(ERROR)<< "Got an invalid json string. Reason: '" << ex.what() << "'.";
                throw invalid_argument(ex.what());
            }
        }
        else
        {
        	// this isn't base64, so it should be an unencrypted recipe
            try {
                recipe_json = json::parse(in);
                LOG(DEBUG)<< "Got a valid, non-encrypted json string.";
            } catch (logic_error& ex) {
                LOG(ERROR)<< "Got an invalid, non-encrypted json string. Reason: '" << ex.what() << "'.";
                throw invalid_argument(ex.what());
            }
        }
#else
        {
            try {
                recipe_json = json::parse(in);
                LOG(DEBUG)<< "Got a valid json string.";
            } catch (logic_error& ex) {
                LOG(ERROR)<< "Got an invalid json string. Reason: '" << ex.what() << "'.";
                throw invalid_argument(ex.what());
            }
        }
#endif
        CheckIngredients(recipe_json["recipe"]);
        max_time = CreateTimeProgram(recipe_json["recipe"], timeprogram_);
    }

    if (max_time > 0) {
        SetPumpControlState(PUMP_STATE_ACTIVE);
        LOG(DEBUG)<< "Successfully imported recipe for product code " << product_id << ".";
        timeprogramrunner_->StartProgram("no order name", timeprogram_);
    }
}

void PumpControl::CheckIngredients(nlohmann::json j) {
    LOG(DEBUG)<< "Checking for availability of the ingredients required by the program.";
    for(auto line: j["lines"].get<vector<json>>()) {
        for(auto component: line["components"].get<vector<json>>()) {
            string ingredient = component["ingredient"].get<string>();
            if(pump_ingredients_bimap_.right.find(ingredient) == pump_ingredients_bimap_.right.end()) {
                stringstream ss;
                ss << "Program requires ingredient " << ingredient << " which isn't available.";
                LOG(ERROR) << ss.str();
                throw out_of_range(ss.str());
            }
        }
    }
    LOG(DEBUG)<< "Successfully checked that all required ingredients are available.";
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
                    SeparateTooFastIngredients(separated_pumps, min_time_map, max_time_map);
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

                        if(find(separated_pumps.begin(), separated_pumps.end(), pump_number) != separated_pumps.end() ||
                                pump_definitions_[pump_number].min_flow == pump_definitions_[pump_number].max_flow) {
                            float flow = pump_definitions_[pump_number].min_flow;
                            timeprogram[time][pump_number] = flow;
                            int end_time = time + amount / flow;
                            timeprogram[end_time][pump_number] = 0;
                        } else {
                            float flow = ((float)amount) / max_duration;
                            timeprogram[time][pump_number] = flow;
                            timeprogram[end_time][pump_number] = 0;
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

int PumpControl::GetMaxElement(const map<int, float>& list) {
    float max = -FLT_MAX;
    int rv = 0;
    for (auto it : list) {
        if (it.second > max) {
            rv = it.first;
            max = it.second;
        }
    }
    return rv;
}

int PumpControl::GetMinElement(const map<int, float>& list) {
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

void PumpControl::SeparateTooFastIngredients(vector<int>& separated_pumps, map<int, float> min_list,
        map<int, float> max_list) {
    int smallest_max_element = GetMinElement(max_list);
    int biggest_min_element = GetMaxElement(min_list);
    LOG(DEBUG)<< "smallest max " << max_list[smallest_max_element] << ", biggest min " << min_list[biggest_min_element];
    if (max_list[smallest_max_element] < min_list[biggest_min_element]) {
        separated_pumps.push_back(smallest_max_element);
        if(1 != min_list.erase(smallest_max_element)){
            throw invalid_argument("internal error: Could not erase element from min_list");
        }
        if(1 != max_list.erase(smallest_max_element)){
            throw invalid_argument("internal error: Could not erase element from max_list");
        }
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

PumpControlInterface::PumpDefinition PumpControl::GetPumpDefinition(size_t pump_number) const {
    const PumpDefinition& pump_definition = pump_definitions_.at(pump_number);
    return pump_definition;
}

float PumpControl::SwitchPump(size_t pump_number, bool switch_on) {
    if(pumpcontrol_state_ != PUMP_STATE_SERVICE){
        throw not_in_this_state("not in service state");
    }
    const PumpDefinition& pump_definition = GetPumpDefinition(pump_number);
    float new_flow = switch_on ? pump_definition.max_flow : 0;
    SetFlow(pump_number, new_flow);
    return new_flow;
}

void PumpControl::StartPumpTimed(size_t pump_number, float rel_current, float duration){
    if(pumpcontrol_state_ != PUMP_STATE_SERVICE){
        throw not_in_this_state("not in service state");
    }
    if((rel_current<0) || (1<rel_current)){
        throw out_of_range("relative current is out of range, range: [0.0 .. 1.0]");
    }
    if((duration<0) || (100<duration)){
        throw out_of_range("duration is out of range, range: [0.0 .. 100.0]");
    }
    pumpdriver_->SetPumpCurrent(pump_number, rel_current);
    usleep(duration*1000000);
    pumpdriver_->SetPumpCurrent(pump_number, 0);
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
    if((pump_number<1) || (8<pump_number)){
        throw out_of_range("pump number is out of range");
    }
    const PumpDefinition& pump_def = pump_definitions_[pump_number];
    float pwm = 0;
    float correctedFlow = flow;
    if(correctedFlow != 0){
        if(correctedFlow < 0){
            throw out_of_range("flow is negative");
        }

        if(correctedFlow < pump_def.min_flow){
            correctedFlow = pump_def.min_flow;
        }else if(correctedFlow > pump_def.max_flow){
            correctedFlow = pump_def.max_flow;
        }
        
        pwm = pump_def.lookup_table[0].pwm_value;
        size_t lookup_entry_count = pump_def.lookup_table.size();
        for(size_t i=1; i<lookup_entry_count; i++){
            if((pump_def.lookup_table[i-1].flow < correctedFlow) && (correctedFlow <= pump_def.lookup_table[i].flow)){
                pwm = (pump_def.lookup_table[i].pwm_value - pump_def.lookup_table[i-1].pwm_value) *
                        (correctedFlow - pump_def.lookup_table[i-1].flow) /
                        (pump_def.lookup_table[i].flow - pump_def.lookup_table[i-1].flow) +
                        pump_def.lookup_table[i-1].pwm_value;
                break;
            }
        }
    }
    pumpdriver_->SetPumpCurrent(pump_number, pwm);
    TrackAmounts(pump_number, correctedFlow);
}

void PumpControl::SetAllPumpsOff(){
    for (auto i : pump_definitions_) {
        SetFlow(i.first, 0);
    }
}

#ifndef NO_ENCRYPTION
void PumpControl::DecryptProgram(unsigned long product_id, const string& in, CryptoBuffer& out){
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
#endif

string PumpControl::GetIoDesc() const{
    string rv;
    io_driver_->GetDesc(rv);
    return rv;
}









