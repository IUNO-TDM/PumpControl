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

  PumpControl * pumpControl = new PumpControl();
  LOG(INFO) << "My first info log using default logger";
  cin.get();

  pumpControl->join();
  delete pumpControl;
  return 0;
}


PumpControl::PumpControl(char* serialPort)
{

  for(auto i: mPumpToIngredientsInit){
    mPumpToIngredientsBiMap.insert(IngredientsBiMap::value_type(i.first, i.second));
  }


  mWebInterface= new WebInterface(9002);
  setClientName("PumpControl");
  mWebInterface->registerCallbackClient(this);
  mWebInterface->start();


  setPumpControlState(PUMP_STATE_IDLE);

  if(mSimulation){
    mPumpDriver = new PumpDriverSimulation();
    printf("The simulation mode is on. Firmata not active!\n");
  }else{
    mPumpDriver = new PumpDriverFirmata();
  }
  
  mPumpDriver->init(NULL);
  mPumpDriver->getPumps(&mPumpDefinitions);
}

PumpControl::PumpControl(){
  PumpControl((char*)STD_SERIAL_PORT[0]);
  mSimulation = false;
}

PumpControl::PumpControl(bool simulation){
  PumpControl((char*)STD_SERIAL_PORT[0]);
  mSimulation = true;
}

PumpControl::~PumpControl(){
  mWebInterface->stop();
  mWebInterface->unregisterCallbackClient(this);
  delete mWebInterface;
  delete mPumpDriver;
}


void PumpControl::start(string receiptJsonString){
  json j = json::parse(receiptJsonString);
  cout << "Successfully imported receipt: " << j["id"] << endl;
  int maxTime = createTimeProgram(j,mTimeProgram);

  for(auto i : mTimeProgram){
    printf("time: %d, onLength: %d, offLength: %d\n", i.first, i.second.onLength, i.second.offLength);

  }

  createTimer(1, maxTime);
}

void PumpControl::join(){
    if (mTimerThread.joinable()){
      mTimerThread.join();
    }
}


int PumpControl::createTimeProgram(json j, TimeProgram &timeProgram){
  int time = 0;
  // for(auto line: j["lines"].get<vector<json>>())
  // {
  //   int maxTime = time;
  //   int sleep = line["sleep"].get<int>();
  //   int timing = line["timing"].get<int>();

  //   if(timing == 0 || timing == 1 || timing == 2){
  //     for(auto component: line["components"].get<std::vector<json>>())
  //     {
  //       string ingredient = component["ingredient"].get<string>();
  //       int amount = component["amount"].get<int>();
  //       this->addOutputToTimeProgram(timeProgram,time, mPumpToIngredientsBiMap.right.count(ingredient), true);
  //       int endTime = time + amount * MS_PER_ML;
  //       this->addOutputToTimeProgram(timeProgram, endTime, mPumpToIngredientsBiMap.right.count(ingredient), false);
  //       if (endTime > maxTime){
  //         maxTime = endTime;
  //       }
  //     }

  //     time = maxTime;
  //   }else if (timing == 3){
  //     for(auto component: line["components"].get<std::vector<json>>())
  //     {
  //       string ingredient = component["ingredient"].get<string>();
  //       int amount = component["amount"].get<int>();
  //       this->addOutputToTimeProgram(timeProgram,time, mPumpToIngredientsBiMap.right.count(ingredient), true);
  //       int endTime = time + amount * MS_PER_ML;
  //       this->addOutputToTimeProgram(timeProgram, endTime, mPumpToIngredientsBiMap.right.count(ingredient), false);
  //       if (endTime > maxTime){
  //         maxTime = endTime;
  //       }

  //       time = maxTime;
  //     }
  //   }

  //   time += sleep + 1;
  // }

  return time;
}

void PumpControl::addOutputToTimeProgram(TimeProgram &timeProgram, int time, int pump, bool on){
  TsTimeCommand timeCommand = timeProgram[time];

  // if(on){
  //   bool found = false;
  //   for(int i = 0; i<timeCommand.onLength; i++)
  //   {
  //     if(timeCommand.outputOn[i] == output){
  //       found = true;
  //       break;
  //     }
  //   }

  //   if(!found){
  //     timeCommand.onLength++;
  //     timeCommand.outputOn[timeCommand.onLength-1] = output;
  //     // printf("Output %d on at %d", output, time);
  //   }
  // }else {
  //   bool found = false;
  //   for(int i = 0; i<timeCommand.offLength; i++)
  //   {
  //     if(timeCommand.outputOff[i] == output){
  //       found = true;
  //       break;
  //     }
  //   }

  //   if(!found){
  //     timeCommand.offLength++;
  //     timeCommand.outputOff[timeCommand.offLength-1] = output;
  //   }
  // }

  timeProgram[time] = timeCommand;
}

void PumpControl::timerWorker(int interval, int maximumTime){

    // int intervals = maximumTime / interval + 1;
    // auto startTime = chrono::system_clock::now();
    // timerFired(0);
    // int currentInterval = 1;
    // while(currentInterval < intervals){
    //   this_thread::sleep_until(startTime + chrono::milliseconds(interval * currentInterval));
    //   timerFired(interval * currentInterval);
    //   currentInterval++;
    // }

    timerEnded();
}

void PumpControl::createTimer(int interval, int maximumTime){
  thread myThread ([this, interval, maximumTime]{
    this->timerWorker(interval, maximumTime);
  });

  mTimerThread = move(myThread);

}

void PumpControl::timerFired(int time)
{

  // if(mTimeProgram.find(time) != mTimeProgram.end() ){
  //     TsTimeCommand timeCommand = mTimeProgram[time];
  //     for(int i = 0; i < timeCommand.offLength; i++){
  //       if(!mSimulation){
  //         firmata_digitalWrite(mFirmata, timeCommand.outputOff[i], LOW);
  //       }else {
  //         printf("Simulate Output %d OFF\n", timeCommand.outputOff[i]);
  //       }
  //     }

  //     for(int i = 0; i < timeCommand.onLength; i++){
  //       if(!mSimulation){
  //         firmata_digitalWrite(mFirmata, timeCommand.outputOn[i], HIGH);
  //       }else {
  //         printf("Simulate Output %d ON\n", timeCommand.outputOn[i]);
  //       }
  //     }
  //  }
}

void PumpControl::timerEnded(){
  printf("Timer ended!\n");
  // if(!mSimulation){
  //   for(auto i : mPumpToOutput){
  //     firmata_digitalWrite(mFirmata, i.second, LOW);
  //   }
  // }

}


bool PumpControl::setPumpControlState(PumpControlState state){
  bool rv = false;
  PumpControlState oldState = mPumpControlState;
  switch(state){
    case PUMP_STATE_ACTIVE:
        if (mPumpControlState == PUMP_STATE_IDLE){
          mPumpControlState = state;
          rv = true;
        }
        break;
    case PUMP_STATE_SERVICE:
      if(mPumpControlState == PUMP_STATE_IDLE){
        mPumpControlState = state;
        rv = true;
      }
      break;
    case PUMP_STATE_UNINITIALIZED:
      break;
    default:
      mPumpControlState = state;
      rv = true;
      break;

  }
  if(rv){
    //send update per Websocket
  }
  return rv;
}

bool PumpControl::httpMessage(std::string method, std::string path, std::string body, http_response_struct *response){
    printf("HTTP Message: Method: %s,Path:%s, Body:%s\n",method.c_str() ,path.c_str() ,body.c_str());
    response->responseCode = 400;
    response->responseMessage = "Ey yo - nix";
    try {
      if (boost::starts_with(path,"/ingredients/"))
      {
        boost::regex expr{"\\/ingredients\\/([0-9]{1,2})"};
        boost::smatch what;
        if(boost::regex_search(path,what,expr)){
          int nr = stoi(what[1].str());
          std::cout << nr<< '\n';
          if (method == "GET"){
            if(mPumpToIngredientsBiMap.left.find(nr) != mPumpToIngredientsBiMap.left.end()){
              response->responseCode = 200;
              response->responseMessage = mPumpToIngredientsBiMap.left.at(nr);
            }else{
              response->responseCode = 404;
              response->responseMessage = "No ingredient for this pump number available!";
            }
          }else if (method == "PUT"){
            if(body.length()> 0){
              auto it = mPumpToIngredientsBiMap.left.find(nr);
              mPumpToIngredientsBiMap.left.replace_data(it,body);
              response->responseCode = 200;
              response->responseMessage = "Successfully stored ingredient for pump";

            }
          }else if (method == "DELETE") {
            if(mPumpToIngredientsBiMap.left.find(nr) != mPumpToIngredientsBiMap.left.end()){
              mPumpToIngredientsBiMap.left.erase(nr);
              response->responseCode = 200;
              response->responseMessage = "Successfully deleted ingredient for pump";
            }else{
              response->responseCode = 404;
              response->responseMessage = "No ingredient for this pump number available!";
            }
          }else {
            response->responseCode = 400;
            response->responseMessage = "Wrong method for this URL";
          }
        }
      }else if (boost::starts_with(path,"/pumps")){
        if (method == "GET"){
            json responseJson;
            for(auto i: mPumpDefinitions){
              responseJson[i.first]["minFlow"] = i.second.minFlow;
              responseJson[i.first]["maxFlow"] = i.second.maxFlow;
              responseJson[i.first]["flowPrecision"] = i.second.flowPrecision;
            }
            response->responseCode = 200;
            response->responseMessage = responseJson.dump();
            
          }
      }else if (boost::starts_with(path,"/service")){
        if (boost::starts_with(path,"/service/pumps/")){
          boost::regex expr{"\\/service\\/pumps\\/([0-9]{1,2})"};
          boost::smatch what;
          if(boost::regex_search(path,what,expr) &&
            method == "PUT" && (body == "true" || body == "false")){
            int nr = stoi(what[1].str());
            if(mPumpControlState == PUMP_STATE_SERVICE){
              if(mPumpDefinitions.find(nr) != mPumpDefinitions.end()){
                //mach jetzt was
                printf("Pump %d should be switched to %s\n",nr,body.c_str() );
                response->responseCode = 200;
                response->responseMessage = "SUCCESS";
              }else{
                response->responseCode = 400;
                response->responseMessage = "Requested Pump not available";
              }
            }else{
              response->responseCode = 400;
              response->responseMessage = "PumpControl not in Service mode";
            }
          }else {
            response->responseCode = 400;
            response->responseMessage = "wrong format to control pumps in service mode";
          }
        }else if(path == "/service" ||path == "/service/" ){
          if(method == "PUT"){
            if(body == "true"){
              if(mPumpControlState == PUMP_STATE_IDLE ){
                bool rv = setPumpControlState(PUMP_STATE_SERVICE);
                if(rv){
                  response->responseCode = 200;
                  response->responseMessage = "Successfully changed to Service state";
                }else {
                  response->responseCode = 500;
                  response->responseMessage = "Could not set Service mode";
                }

              }else if (mPumpControlState == PUMP_STATE_SERVICE) {
                response->responseCode = 300;
                response->responseMessage = "is already Service state";
              }else {
                response->responseCode = 400;
                response->responseMessage = "Currently in wrong state for Service";
              }
            }else if (body == "false"){
              if( mPumpControlState == PUMP_STATE_SERVICE){
                bool rv = setPumpControlState(PUMP_STATE_IDLE);
                if(rv){
                  response->responseCode = 200;
                  response->responseMessage = "Successfully changed to Idle state";
                }else {
                  response->responseCode = 500;
                  response->responseMessage = "Could not set idle mode";
                }
              }else if (mPumpControlState == PUMP_STATE_IDLE) {
                response->responseCode = 300;
                response->responseMessage = "is already idle state";
              }else {
                response->responseCode = 400;
                response->responseMessage = "Currently in wrong state for idle";
              }
            }else {
              response->responseCode = 400;
              response->responseMessage = "incorrect parameter";
            }

          }else {
            response->responseCode = 400;
            response->responseMessage = "servicemode can only be set or unset.\
            No other function available. watch websocket!";
          }
        }
      }else if (path == "/program"){
        if(method == "PUT"){
          if(mPumpControlState == PUMP_STATE_IDLE){
            //check Program
            printf("Program should be loaded: %s\n",body.c_str() );
            response->responseCode = 200;
            response->responseMessage = "SUCCESS";
          }else {
            response->responseCode = 400;
            response->responseMessage = "PumpControl must be in mode idle to upload a program";
          }
        }else{
          response->responseCode = 400;
          response->responseMessage = "wrong method for program upload";
        }
      }else{
        response->responseCode = 400;
        response->responseMessage = "unrecognized path";
      }
    } catch (boost::bad_lexical_cast) {
        // bad parameter
    }



    return true;
}
bool PumpControl::webSocketMessage(std::string message, std::string * response){
    *response = "bla";
    return true;
}
