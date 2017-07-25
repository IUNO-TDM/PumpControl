#include "pumpcontrol.h"
#include "pumpdriversimulation.h"
#include "pumpdriverfirmata.h"

#include "easylogging++.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace nlohmann;

PumpControl::PumpControl(string serial_port, bool simulation, int websocket_port,
        map<int, PumpDriverInterface::PumpDefinition> pump_definitions) {
    Init(serial_port, simulation, websocket_port, pump_definitions);
}

PumpControl::~PumpControl() {
    LOG(DEBUG)<< "PumpControl destructor";
    if (webinterface_) {
        webinterface_->Stop();
        webinterface_->UnregisterCallbackClient(this);
        delete webinterface_;
    }
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

void PumpControl::Init(string serial_port, bool simulation, int websocket_port,
        map<int, PumpDriverInterface::PumpDefinition> pump_definitions) {
    simulation_ = simulation;
    pump_definitions_ = pump_definitions;
    serialport_ = serial_port;

    for (auto i : kPumpIngredientsInit) {
        pump_ingredients_bimap_.insert(boost::bimap<int, string>::value_type(i.first, i.second));
    }

    webinterface_ = new WebInterface(websocket_port);
    webinterface_->RegisterCallbackClient(this);
    bool success = webinterface_->Start();

    if (success) {
        SetPumpControlState(PUMP_STATE_IDLE);
        string config_string;
        if (simulation_) {
            pumpdriver_ = new PumpDriverSimulation();
            LOG(INFO)<< "The simulation mode is on. Firmata not active!";
            config_string = "simulation";

        } else {
            pumpdriver_ = new PumpDriverFirmata();
            config_string = serialport_;
        }

        success = pumpdriver_->Init(config_string.c_str(), pump_definitions_, this);
        if (success) {
            timeprogramrunner_ = new TimeProgramRunner(this, pumpdriver_);
            timeprogramrunner_thread_ = thread(&TimeProgramRunner::Run, timeprogramrunner_);
        } else {
            SetPumpControlState(PUMP_STATE_ERROR);
            LOG(ERROR)<< "Could not initialize the Pump Driver. You can now close the application.";
        }

    } else {
        LOG(ERROR)<< "Could not inititalize the WebInterface. You can now close the application.";
    }

}

bool PumpControl::Start(const char* recipe_json_string) {
    bool success = false;
    success = SetPumpControlState(PUMP_STATE_ACTIVE);
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

    return success;
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
        json json_message = json::object();
        json_message["error"]["type"] = "TimeProgramParseError";
        json_message["error"]["nr"] = 1;
        json_message["error"]["details"] = ex.what();
        webinterface_->SendMessage(json_message.dump());
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
    LOG(DEBUG)<< "smallest max " << max_list[smallest_max_element] << "bigges min " << min_list[biggest_min_element];
    if (max_list[smallest_max_element] < min_list[biggest_min_element]) {
        separated_pumps->push_back(smallest_max_element);
        min_list.erase(smallest_max_element);
        max_list.erase(smallest_max_element);
        SeparateTooFastIngredients(separated_pumps, min_list, max_list);
    }
}

bool PumpControl::SetPumpControlState(PumpControlState state) {
    bool rv = false;
    LOG(DEBUG)<< "SetPumpControlState: " << NameForPumpControlState(state);
    switch (state) {
        case PUMP_STATE_ACTIVE:
            if (pumpcontrol_state_ == PUMP_STATE_IDLE) {
                pumpcontrol_state_ = state;
                rv = true;
            }
            break;
        case PUMP_STATE_SERVICE:
            if (pumpcontrol_state_ == PUMP_STATE_IDLE) {
                pumpcontrol_state_ = state;
                rv = true;
            }
            break;
        case PUMP_STATE_UNINITIALIZED:
            break;
        default:
            pumpcontrol_state_ = state;
            rv = true;
            break;

    }
    if (rv) {
        LOG(DEBUG)<< "send update to websocketclients: " << NameForPumpControlState(state);
        json json_message = json::object();
        json_message["mode"] = NameForPumpControlState(pumpcontrol_state_);
        webinterface_->SendMessage(json_message.dump());
    }
    return rv;
}

const char* PumpControl::NameForPumpControlState(PumpControlState state) {
    switch (state) {
        case PUMP_STATE_ACTIVE:
            return "active";
        case PUMP_STATE_ERROR:
            return "error";
        case PUMP_STATE_IDLE:
            return "idle";
        case PUMP_STATE_SERVICE:
            return "service";
        case PUMP_STATE_UNINITIALIZED:
            return "uninitialized";
    }
    return "internal problem";
}

bool PumpControl::WebInterfaceHttpMessage(string method, string path, string body,
        HttpResponse *response) {
    printf("HTTP Message: Method: %s,Path:%s, Body:%s\n", method.c_str(), path.c_str(), body.c_str());
    response->response_code = 400;
    response->response_message = "Ey yo - nix";
    try {
        if (boost::starts_with(path, "/ingredients/")) {
            boost::regex expr { "\\/ingredients\\/([0-9]{1,2}(\\/amount)?)" };
            boost::smatch what;
            if (boost::regex_search(path, what, expr)) {
                int nr = stoi(what[1].str());
                if (what.size() == 3 && what[2].str() == "/amount") {
                    if (method == "PUT") {
                        if (body.length() > 0) {
                            auto it = pump_ingredients_bimap_.left.find(nr);
                            if (it != pump_ingredients_bimap_.left.end()) {
                                try {
                                    int amount = atoi(body.c_str());
                                    pumpdriver_->SetAmountForPump(nr, amount);
                                    LOG(DEBUG)<< amount << "ml has been set for pump " << nr;
                                    response->response_code = 200;
                                    response->response_message = "Successfully stored amount for ingredient for pump";
                                } catch (exception& e) {
                                    LOG(ERROR)<< e.what();
                                    response->response_code = 400;
                                    response->response_message = "The body is invalid";
                                }
                            } else {
                                LOG(DEBUG)<< "Amount for pump nr " << nr << "cant be set, because it is not configured";
                                response->response_code = 400;
                                response->response_message = "This pump is not configured";
                            }
                        } else {
                            response->response_code = 400;
                            response->response_message = "Body is empty";
                        }
                    } else {
                        response->response_code = 400;
                        response->response_message = "Wrong method for this URL";
                    }
                } else {
                    if (method == "GET") {
                        if(pump_ingredients_bimap_.left.find(nr) != pump_ingredients_bimap_.left.end()) {
                            response->response_code = 200;
                            response->response_message = pump_ingredients_bimap_.left.at(nr);
                        } else {
                            response->response_code = 404;
                            response->response_message = "No ingredient for this pump number available!";
                        }
                    } else if (method == "PUT") {
                        if(body.length()> 0) {
                            auto it = pump_ingredients_bimap_.left.find(nr);
                            pump_ingredients_bimap_.left.replace_data(it,body);
                            LOG(DEBUG) << "Pump number " << nr << "is now bound to "<< body;
                            response->response_code = 200;
                            response->response_message = "Successfully stored ingredient for pump";

                        }
                    } else if (method == "DELETE") {
                        if(pump_ingredients_bimap_.left.find(nr) != pump_ingredients_bimap_.left.end()) {
                            pump_ingredients_bimap_.left.erase(nr);
                            response->response_code = 200;
                            response->response_message = "Successfully deleted ingredient for pump";
                        } else {
                            response->response_code = 404;
                            response->response_message = "No ingredient for this pump number available!";
                        }
                    } else {
                        response->response_code = 400;
                        response->response_message = "Wrong method for this URL";
                    }
                }
            }
        } else if (boost::starts_with(path,"/pumps")) {
            if (method == "GET") {
                json responseJson = json::object();
                for(auto i: pump_definitions_) {
                    char key[3];
                    sprintf(key,"%d",i.first);
                    json pump = json::object();
                    pump["minFlow"] = i.second.min_flow;
                    pump["maxFlow"] = i.second.max_flow;
                    pump["flowPrecision"] = i.second.flow_precision;
                    responseJson[key]= pump;
                }
                response->response_code = 200;
                response->response_message = responseJson.dump();

            }
        } else if (boost::starts_with(path,"/service")) {
            if (boost::starts_with(path,"/service/pumps/")) {
                boost::regex expr {"\\/service\\/pumps\\/([0-9]{1,2})"};
                boost::smatch what;
                if(boost::regex_search(path,what,expr) &&
                        method == "PUT" && (body == "true" || body == "false")) {
                    int nr = stoi(what[1].str());
                    if(pumpcontrol_state_ == PUMP_STATE_SERVICE) {
                        auto it = pump_definitions_.find(nr);
                        if(it != pump_definitions_.end()) {
                            pumpdriver_->SetPump(nr,body=="true"?it->second.max_flow:0);
                            printf("Pump %d should be switched to %s\n",nr,body.c_str() );
                            response->response_code = 200;
                            response->response_message = "SUCCESS";

                            json json_message = json::object();
                            json_message["service"]["pump"] = nr;
                            json_message["service"]["flow"] = body=="true"?it->second.max_flow:0;
                            webinterface_->SendMessage(json_message.dump());
                        } else {
                            response->response_code = 400;
                            response->response_message = "Requested Pump not available";
                        }
                    } else {
                        response->response_code = 400;
                        response->response_message = "PumpControl not in Service mode";
                    }
                } else {
                    response->response_code = 400;
                    response->response_message = "wrong format to control pumps in service mode";
                }
            } else if (path == "/service" ||path == "/service/" ) {
                if (method == "PUT") {
                    if (body == "true") {
                        if (pumpcontrol_state_ == PUMP_STATE_IDLE ) {
                            bool rv = SetPumpControlState(PUMP_STATE_SERVICE);
                            if(rv) {
                                response->response_code = 200;
                                response->response_message = "Successfully changed to Service state";
                            } else {
                                response->response_code = 500;
                                response->response_message = "Could not set Service mode";
                            }

                        } else if (pumpcontrol_state_ == PUMP_STATE_SERVICE) {
                            response->response_code = 300;
                            response->response_message = "is already Service state";
                        } else {
                            response->response_code = 400;
                            response->response_message = "Currently in wrong state for Service";
                        }
                    } else if (body == "false") {
                        if( pumpcontrol_state_ == PUMP_STATE_SERVICE) {
                            bool rv = SetPumpControlState(PUMP_STATE_IDLE);
                            if(rv) {
                                response->response_code = 200;
                                response->response_message = "Successfully changed to Idle state";
                            } else {
                                response->response_code = 500;
                                response->response_message = "Could not set idle mode";
                            }
                        } else if (pumpcontrol_state_ == PUMP_STATE_IDLE) {
                            response->response_code = 300;
                            response->response_message = "is already idle state";
                        } else {
                            response->response_code = 400;
                            response->response_message = "Currently in wrong state for idle";
                        }
                    } else {
                        response->response_code = 400;
                        response->response_message = "incorrect parameter";
                    }

                } else {
                    response->response_code = 400;
                    response->response_message = "servicemode can only be set or unset. "
                                "No other function available. watch websocket!";
                }
            }
        } else if (path == "/program") {
            if(method == "PUT") {
                if(pumpcontrol_state_ == PUMP_STATE_IDLE) {
                    if(Start(body.c_str())) {
                        response->response_code = 200;
                        response->response_message = "SUCCESS";
                    } else {
                        response->response_code = 500;
                        response->response_message = "Could not start the Program";
                    }
                } else {
                    response->response_code = 400;
                    response->response_message = "PumpControl must be in mode idle to upload a program";
                }
            } else {
                response->response_code = 400;
                response->response_message = "wrong method for program upload";
            }
        } else {
            response->response_code = 400;
            response->response_message = "unrecognized path";
        }
    } catch (boost::bad_lexical_cast&) {
        // bad parameter
    }

    return true;
}

void PumpControl::WebInterfaceOnOpen() {
    json json_message = json::object();
    json_message["mode"] = NameForPumpControlState(pumpcontrol_state_);
    webinterface_->SendMessage(json_message.dump());
}

void PumpControl::TimeProgramRunnerProgressUpdate(string id, int percent) {
    LOG(DEBUG)<< "TimeProgramRunnerProgressUpdate " << percent << " : " << id;
    json json_message = json::object();
    json_message["progressUpdate"]["orderName"] = id;
    json_message["progressUpdate"]["progress"] = percent;
    webinterface_->SendMessage(json_message.dump());

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
    json json_message = json::object();
    json_message["programEnded"]["orderName"] = id;
    webinterface_->SendMessage(json_message.dump());
    timeprogram_.clear();
}

void PumpControl::PumpDriverAmountWarning(int pump_number, int amountWarningLimit) {
    string ingredient = pump_ingredients_bimap_.left.at(pump_number);

    LOG(DEBUG)<< "PumpDriverAmountWarning: nr:" << pump_number << " ingredient: " << ingredient << " Amount warning level: " << amountWarningLimit;
    json json_message = json::object();
    json_message["amountWarning"]["pumpNr"] = pump_number;
    json_message["amountWarning"]["ingredient"] = ingredient;
    json_message["amountWarning"]["amountWarningLimit"] = amountWarningLimit;
    webinterface_->SendMessage(json_message.dump());
}
