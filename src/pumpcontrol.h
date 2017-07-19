
#include "firmata.h"
#include "json.hpp"
#include <unistd.h>
#include <map>
#include <vector>
#include <thread>
#include "webinterface.h"
#include "easylogging++.h"
#include <boost/bimap.hpp>
#include "pumpdriverinterface.h"
#include "timeprogramrunner.h"

class PumpControl: public WebInterfaceCallbackClient, public TimeProgramRunnerCallback, public PumpDriverCallbackClient{

  public:
    enum PumpControlState{
      PUMP_STATE_UNINITIALIZED = 0,
      PUMP_STATE_IDLE = 1,
      PUMP_STATE_ACTIVE = 2,
      PUMP_STATE_SERVICE = 3,
      PUMP_STATE_ERROR = 4
    };

    // PumpControl();
    PumpControl(std::string serial_port, bool simulation, int websocket_port, std::map<int,PumpDriverInterface::PumpDefinition> pump_configurations);
    // PumpControl(bool simulation);
    virtual ~PumpControl();

    //WebInterfaceCallbackClient
    bool WebInterfaceHttpMessage(std::string method, std::string path, std::string body, HttpResponse *response);
    bool WebInterfaceWebSocketMessage(std::string message, std::string * response);
    void WebInterfaceOnOpen();
    //TimeProgramRunnerCallback
    void TimeProgramRunnerProgressUpdate(std::string id,int percent);
    void TimeProgramRunnerStateUpdate(TimeProgramRunner::TimeProgramRunnerState state);
    void TimeProgramRunnerProgramEnded(std::string id);
    void PumpDriverAmountWarning(int pump_number, int amount_left);
    // void TimeProgramRunnerAmountWarning(int pump_number, int amount_left);

  private:
    PumpControlState pumpcontrol_state_ = PUMP_STATE_UNINITIALIZED;
    PumpDriverInterface *pumpdriver_ = NULL;
    std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions_;
    
    WebInterface *webinterface_ = NULL;
    TimeProgramRunner *timeprogramrunner_ = NULL;

    TimeProgramRunner::TimeProgram timeprogram_;

    std::string serialport_;
    bool simulation_;
    
    boost::bimap<int,std::string> pump_ingredients_bimap_;
    const std::map<int,std::string> kPumpIngredientsInit  {
       { 1, "Orangensaft" },
       { 2, "Apfelsaft" },
       { 3, "Sprudel" },
       { 4, "Kirschsaft" },
       { 5, "Bananensaft" },
       { 6, "Johannisbeersaft" },
       { 7, "Cola" },
       { 8, "Fanta" }};

        
    std::thread timeprogramrunner_thread_;

    int CreateTimeProgram(nlohmann::json j, TimeProgramRunner::TimeProgram &timeprogram);;
    void TimerWorker(int interval, int maximumTime);
    void CreateTimer(int interval, int maximumTime);
    void TimerFired( int time);
    void TimerEnded();
    bool SetPumpControlState(PumpControlState state);
    void Init(std::string serial_port, bool simulation, int websocket_port, std::map<int,PumpDriverInterface::PumpDefinition> pump_configurations);
    bool Start(const char* receipt_json_string);
    const char *NameForPumpControlState(PumpControlState state);
    int GetMaxElement(std::map<int,float> list);
    int GetMinElement(std::map<int, float> list);
    void SeparateTooFastIngredients(std::vector<int> *separated_pumps, std::map<int, float> min_list, std::map<int, float> max_list);
};
