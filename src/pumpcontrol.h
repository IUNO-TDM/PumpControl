
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

class PumpControl: public WebInterfaceCallbackClient, public TimeProgramRunnerCallback{

   

  public:
    typedef enum {
      PUMP_STATE_UNINITIALIZED = 0,
      PUMP_STATE_IDLE = 1,
      PUMP_STATE_ACTIVE = 2,
      PUMP_STATE_SERVICE = 3,
      PUMP_STATE_ERROR = 4
    }PumpControlState;

    PumpControl();
    PumpControl(const char* serialPort);
    PumpControl(bool simulation);
    ~PumpControl();

    //WebInterfaceCallbackClient
    bool WebInterfaceHttpMessage(std::string method, std::string path, std::string body, HttpResponse *response);
    bool WebInterfaceWebSocketMessage(std::string message, std::string * response);
    //TimeProgramRunnerCallback
    void TimeProgramRunnerProgressUpdate(const char* id,int percent);
    void TimeProgramRunnerStateUpdate(TimeProgramRunner::TimeProgramRunnerState state);
    void TimeProgramRunnerProgramEnded(const char* id);

  private:
    PumpControlState pumpcontrol_state_ = PUMP_STATE_UNINITIALIZED;
    PumpDriverInterface * pumpdriver_;
    std::map<int,PumpDriverInterface::PumpDefinition> pumpdefinitions_;
    WebInterface * webinterface_;
    TimeProgramRunner * timeprogramrunner_;

    TimeProgramRunner::TimeProgram timeprogram_;

    char* serialport_;
    bool simulation_;
    
    const char * STD_SERIAL_PORT = "/dev/tty.usbserial-A104WO1O";
    typedef boost::bimap<int,std::string> IngredientsBiMap;
    IngredientsBiMap pump_ingredients_bimap_;
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

    int CreateTimeProgram(nlohmann::json j, TimeProgramRunner::TimeProgram &timeprogram);
    // void addOutputToTimeProgram(TimeProgram &timeProgram, int time, int pump, bool on);
    void TimerWorker(int interval, int maximumTime);
    void CreateTimer(int interval, int maximumTime);
    void TimerFired( int time);
    void TimerEnded();
    bool SetPumpControlState(PumpControlState state);
    void Init(const char* serial_port, bool simulation);
    bool Start(const char* receipt_json_string);
    const char* NameForPumpControlState(PumpControlState state);
    int GetMaxElement(std::map<int,float> list);
    int GetMinElement(std::map<int, float> list);
    void SeparateTooFastIngredients(std::vector<int> *separated_pumps, std::map<int, float> min_list, std::map<int, float> max_list);
};
