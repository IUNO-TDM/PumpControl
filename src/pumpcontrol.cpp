#include "pumpcontrol.h"
#include <chrono>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <csignal>
#include "pumpdriversimulation.h"
#include "pumpdriverfirmata.h"

using namespace std;
using namespace nlohmann;


INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{

  PumpControl *pump_control = new PumpControl(false);
  LOG(INFO) << "My first info log using default logger";
  cin.get();
  
  delete pump_control;
  LOG(INFO) << "Application closes now";
  return 0;
}

PumpControl::PumpControl(const char* serialPort)
{
  Init(serialPort, false);
}

PumpControl::PumpControl(){
  Init(STD_SERIAL_PORT, false);
}

PumpControl::PumpControl(bool simulation){
  Init(STD_SERIAL_PORT, simulation);
}

PumpControl::~PumpControl(){
  LOG(DEBUG) << "PumpControl destructor";
  webinterface_->Stop();
  webinterface_->UnregisterCallbackClient(this);
  timeprogramrunner_->Shutdown();
  timeprogramrunner_thread_.join();
  delete timeprogramrunner_;
  delete webinterface_;
  delete pumpdriver_;
  LOG(DEBUG) << "PumpControl destructor finished";
}


void PumpControl::Init(const char* serialPort, bool simulation){
  simulation_ = simulation;
  uint len = strlen(serialPort);
  serialport_ = new char(len);
  strcpy(serialport_, serialPort);

  for(auto i: kPumpIngredientsInit){
    pump_ingredients_bimap_.insert(IngredientsBiMap::value_type(i.first, i.second));
  }


  webinterface_= new WebInterface(9002);
  SetClientName("PumpControl");
  webinterface_->RegisterCallbackClient(this);
  webinterface_->Start();


  SetPumpControlState(PUMP_STATE_IDLE);
  string config_string;
  if(simulation_){
    pumpdriver_ = new PumpDriverSimulation();
    LOG(INFO) << "The simulation mode is on. Firmata not active!";
    config_string = "simulation";

  }else{
    pumpdriver_ = new PumpDriverFirmata();
    config_string = "/dev/tty.usbserial-A104WO1O";
  }
  
  pumpdriver_->Init(config_string.c_str());
  pumpdriver_->GetPumps(&pumpdefinitions_);
  timeprogramrunner_ = new TimeProgramRunner(this,pumpdriver_);
  timeprogramrunner_thread_ = thread(&TimeProgramRunner::Run,timeprogramrunner_);
}

bool PumpControl::Start(const char*  recipe_json_string){
  bool success = false;
  success = SetPumpControlState(PUMP_STATE_ACTIVE);
  json j = json::parse(recipe_json_string);
  LOG(DEBUG) << "Successfully imported recipe: " << j["id"];
  int max_time = CreateTimeProgram(j,timeprogram_);

  timeprogramrunner_->StartProgram(timeprogram_);
  
  return success;
}



int PumpControl::CreateTimeProgram(json j, TimeProgramRunner::TimeProgram &timeprogram){
  LOG(DEBUG) << "createTimeProgram";
  int time = 0;
  for(auto line: j["lines"].get<vector<json>>())
  {
    int max_time = time;
    int sleep = line["sleep"].get<int>();
    int timing = line["timing"].get<int>();

    if(timing == 0 || timing == 1 || timing == 2){
      for(auto component: line["components"].get<std::vector<json>>())
      {
        string ingredient = component["ingredient"].get<string>();
        int amount = component["amount"].get<int>();
        int pump_number = pump_ingredients_bimap_.right.at(ingredient);
        float max_flow =  pumpdefinitions_[pump_number].max_flow;
        timeprogram[time][pump_number] = max_flow;
        int end_time = time + amount / max_flow;
        timeprogram[end_time][pump_number] = 0;
        
        if (end_time > max_time){
          max_time = end_time;
        }
      }

      time = max_time;
    } else if (timing == 3){
      for(auto component: line["components"].get<std::vector<json>>())
      {
        string ingredient = component["ingredient"].get<string>();
        int amount = component["amount"].get<int>();
        int pump_number = pump_ingredients_bimap_.right.at(ingredient);
        float max_flow =  pumpdefinitions_[pump_number].max_flow;
        timeprogram[time][pump_number] = max_flow;
        int end_time = time + amount / max_flow;
        timeprogram[end_time][pump_number] = 0;
        
        if (end_time > max_time){
          max_time = end_time;
        }

        time = max_time;
      }
    }

    time += sleep + 1;
  }

  return time;
}



bool PumpControl::SetPumpControlState(PumpControlState state){
  bool rv = false;
  LOG(DEBUG) << "SetPumpControlState: " << NameForPumpControlState(state);
  switch(state){
    case PUMP_STATE_ACTIVE:
        if (pumpcontrol_state_ == PUMP_STATE_IDLE){
          pumpcontrol_state_ = state;
          rv = true;
        }
        break;
    case PUMP_STATE_SERVICE:
      if(pumpcontrol_state_ == PUMP_STATE_IDLE){
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
  if(rv){
    LOG(DEBUG) << "send update to websocketclients: " << NameForPumpControlState(state);
    json json_message = json::object();
    json_message["mode"] = NameForPumpControlState(pumpcontrol_state_);
    webinterface_->SendMessage(json_message.dump());
  }
  return rv;
}

const char* PumpControl::NameForPumpControlState(PumpControlState state){
  switch(state){
    case PUMP_STATE_ACTIVE: return "active";
    case PUMP_STATE_ERROR: return "error";
    case PUMP_STATE_IDLE: return "idle";
    case PUMP_STATE_SERVICE: return "service";
    case PUMP_STATE_UNINITIALIZED: return "uninitialized";
  }
  return "internal problem";
}

bool PumpControl::WebInterfaceHttpMessage(std::string method, std::string path, std::string body, HttpResponse *response){
    printf("HTTP Message: Method: %s,Path:%s, Body:%s\n",method.c_str() ,path.c_str() ,body.c_str());
    response->response_code = 400;
    response->response_message = "Ey yo - nix";
    try {
      if (boost::starts_with(path,"/ingredients/"))
      {
        boost::regex expr{"\\/ingredients\\/([0-9]{1,2})"};
        boost::smatch what;
        if(boost::regex_search(path,what,expr)){
          int nr = stoi(what[1].str());
          std::cout << nr<< '\n';
          if (method == "GET"){
            if(pump_ingredients_bimap_.left.find(nr) != pump_ingredients_bimap_.left.end()){
              response->response_code = 200;
              response->response_message = pump_ingredients_bimap_.left.at(nr);
            }else{
              response->response_code = 404;
              response->response_message = "No ingredient for this pump number available!";
            }
          }else if (method == "PUT"){
            if(body.length()> 0){
              auto it = pump_ingredients_bimap_.left.find(nr);
              pump_ingredients_bimap_.left.replace_data(it,body);
              response->response_code = 200;
              response->response_message = "Successfully stored ingredient for pump";

            }
          }else if (method == "DELETE") {
            if(pump_ingredients_bimap_.left.find(nr) != pump_ingredients_bimap_.left.end()){
              pump_ingredients_bimap_.left.erase(nr);
              response->response_code = 200;
              response->response_message = "Successfully deleted ingredient for pump";
            }else{
              response->response_code = 404;
              response->response_message = "No ingredient for this pump number available!";
            }
          }else {
            response->response_code = 400;
            response->response_message = "Wrong method for this URL";
          }
        }
      }else if (boost::starts_with(path,"/pumps")){
        if (method == "GET"){
            json responseJson = json::object();
            for(auto i: pumpdefinitions_){
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
      } else if (boost::starts_with(path,"/service")){
        if (boost::starts_with(path,"/service/pumps/")){
          boost::regex expr{"\\/service\\/pumps\\/([0-9]{1,2})"};
          boost::smatch what;
          if(boost::regex_search(path,what,expr) &&
            method == "PUT" && (body == "true" || body == "false")){
            int nr = stoi(what[1].str());
            if(pumpcontrol_state_ == PUMP_STATE_SERVICE){
              auto it = pumpdefinitions_.find(nr);
              if(it != pumpdefinitions_.end()){
                pumpdriver_->SetPump(nr,body=="true"?it->second.max_flow:0);
                printf("Pump %d should be switched to %s\n",nr,body.c_str() );
                response->response_code = 200;
                response->response_message = "SUCCESS";
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
          if (method == "PUT"){
            if (body == "true"){
              if (pumpcontrol_state_ == PUMP_STATE_IDLE ) {
                bool rv = SetPumpControlState(PUMP_STATE_SERVICE);
                if(rv){
                  response->response_code = 200;
                  response->response_message = "Successfully changed to Service state";
                }else {
                  response->response_code = 500;
                  response->response_message = "Could not set Service mode";
                }

              }else if (pumpcontrol_state_ == PUMP_STATE_SERVICE) {
                response->response_code = 300;
                response->response_message = "is already Service state";
              }else {
                response->response_code = 400;
                response->response_message = "Currently in wrong state for Service";
              }
            }else if (body == "false"){
              if( pumpcontrol_state_ == PUMP_STATE_SERVICE){
                bool rv = SetPumpControlState(PUMP_STATE_IDLE);
                if(rv){
                  response->response_code = 200;
                  response->response_message = "Successfully changed to Idle state";
                }else {
                  response->response_code = 500;
                  response->response_message = "Could not set idle mode";
                }
              }else if (pumpcontrol_state_ == PUMP_STATE_IDLE) {
                response->response_code = 300;
                response->response_message = "is already idle state";
              }else {
                response->response_code = 400;
                response->response_message = "Currently in wrong state for idle";
              }
            }else {
              response->response_code = 400;
              response->response_message = "incorrect parameter";
            }

          }else {
            response->response_code = 400;
            response->response_message = "servicemode can only be set or unset.\
            No other function available. watch websocket!";
          }
        }
      }else if (path == "/program"){
        if(method == "PUT"){
          if(pumpcontrol_state_ == PUMP_STATE_IDLE){
            if(Start(body.c_str())){
              response->response_code = 200;
              response->response_message = "SUCCESS";
            }else{
              response->response_code = 500;
              response->response_message = "Could not start the Program";  
            }
          }else {
            response->response_code = 400;
            response->response_message = "PumpControl must be in mode idle to upload a program";
          }
        }else{
          response->response_code = 400;
          response->response_message = "wrong method for program upload";
        }
      }else{
        response->response_code = 400;
        response->response_message = "unrecognized path";
      }
    } catch (boost::bad_lexical_cast) {
        // bad parameter
    }



    return true;
}
bool PumpControl::WebInterfaceWebSocketMessage(std::string message, std::string * response){
    *response = "bla";
    return true;
}

void PumpControl::TimeProgramRunnerProgressUpdate(int percent){
  LOG(DEBUG) << "TimeProgramRunnerProgressUpdate " << percent; 
  json json_message = json::object();
  json_message["progress"] = percent;
  webinterface_->SendMessage(json_message.dump());

}
void PumpControl::TimeProgramRunnerStateUpdate(TimeProgramRunner::TimeProgramRunnerState state){
  LOG(DEBUG) << "TimeProgramRunnerStateUpdate " << TimeProgramRunner::NameForState(state);

  switch(state){
    case TimeProgramRunner::TimeProgramRunnerState::TIME_PROGRAM_IDLE:
      SetPumpControlState(PUMP_STATE_IDLE);
      break;
    case  TimeProgramRunner::TimeProgramRunnerState::TIME_PROGRAM_ACTIVE:
       SetPumpControlState(PUMP_STATE_ACTIVE);
      break;
    default:
      //do nothing
      break;
  }
}
void PumpControl::TimeProgramRunnerProgramEnded(){
  LOG(DEBUG) << "TimeProgramRunnerProgramEnded";
}