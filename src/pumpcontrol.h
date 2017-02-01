
#include "firmata.h"
#include "json.hpp"
#include <unistd.h>
#include <map>
#include <thread>
#include "webinterface.h"
#include "easylogging++.h"
#include <boost/bimap.hpp>
#include "pumpdriverinterface.h"

class PumpControl: public WebInterfaceCallbackClient{

  #define STD_SERIAL_PORT "/dev/tty.usbserial-A104WO1O"

  public:
    typedef enum {
      PUMP_STATE_UNINITIALIZED = 0,
      PUMP_STATE_IDLE = 1,
      PUMP_STATE_ACTIVE = 2,
      PUMP_STATE_SERVICE = 3,
      PUMP_STATE_ERROR = 4
    }PumpControlState;

    PumpControl();
    PumpControl(char* serialPort);
    PumpControl(bool simulation);
    ~PumpControl();
    void start(std::string receiptJsonString);
    void join();

    bool httpMessage(std::string method, std::string path, std::string body, http_response_struct *response);
    bool webSocketMessage(std::string message, std::string * response);

  private:
    PumpControlState mPumpControlState = PUMP_STATE_UNINITIALIZED;
    PumpDriverInterface * mPumpDriver;
    std::map<int,PumpDriverInterface::PumpDefinition> mPumpDefinitions;
    WebInterface * mWebInterface;
    typedef struct {
      int onLength = 0;
      int offLength = 0;
      int outputOn[8];
      int outputOff[8];
    }TsTimeCommand;
    typedef std::map<int,TsTimeCommand> TimeProgram;

    TimeProgram mTimeProgram;


    char* mSerialPort;
    bool mSimulation;
    char* mProductId;

    typedef boost::bimap<int,std::string> IngredientsBiMap;
    IngredientsBiMap mPumpToIngredientsBiMap;
    std::map<int,std::string> mPumpToIngredientsInit  {
       { 1, "Orangensaft" },
       { 2, "Apfelsaft" },
       { 3, "Sprudel" },
       { 4, "Kirschsaft" },
       { 5, "Bananensaft" },
       { 6, "Johannisbeersaft" },
       { 7, "Cola" },
       { 8, "Fanta" }};
    
    std::thread mTimerThread;

    int createTimeProgram(nlohmann::json j, TimeProgram &timeProgram);
    void addOutputToTimeProgram(TimeProgram &timeProgram, int time, int pump, bool on);
    void timerWorker(int interval, int maximumTime);
    void createTimer(int interval, int maximumTime);
    void timerFired( int time);
    void timerEnded();
    bool setPumpControlState(PumpControlState state);

};
