#ifndef PUMPDRIVERINTERFACE_H
#define PUMPDRIVERINTERFACE_H
#include <unistd.h>
#include <map>

class PumpDriverCallbackClient{
    public:
        virtual ~PumpDriverCallbackClient(){}
        virtual void PumpDriverAmountWarning(int pump_number, int amountWarningLimit) = 0;
};

class PumpDriverInterface {
public:
    struct PumpDefinition{
      int output;
      //the minimum possible flow in ml/s
      float min_flow;
      //the aximum possible flow in ml/s
      float max_flow;
      //the flow precision in ml/s
      float flow_precision;
    };

    virtual ~PumpDriverInterface(){};

    virtual bool Init(const char* config_text_ptr, std::map<int, PumpDefinition> pump_definitions, PumpDriverCallbackClient* callbackClient) = 0;

    virtual void DeInit() = 0;

    virtual int GetPumpCount() = 0;

    virtual void SetPump(int pump_number, float flow) = 0;

    virtual void SetAmountForPump(int pump_number, int amount) = 0;
};

#endif
