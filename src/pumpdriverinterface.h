#ifndef PUMPDRIVERINTERFACE_H
#define PUMPDRIVERINTERFACE_H
#include <unistd.h>
#include <map>

class PumpDriverInterface {
public:
    typedef struct{
      //the minimum possible flow in ml/s
      float min_flow;
      //the maximum possible flow in ml/s
      float max_flow;
      //the flow precision in ml/s
      float flow_precision;
    } PumpDefinition;

    virtual ~PumpDriverInterface(void){};

    virtual void Init(const char* config_text_ptr) = 0;

    virtual void DeInit() = 0;

    virtual void GetPumps(std::map<int,PumpDefinition>* pump_definitions) = 0;

    virtual void SetPump(int pump_number, float flow) = 0;
};
#endif