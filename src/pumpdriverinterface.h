#ifndef PUMPDRIVERINTERFACE_H
#define PUMPDRIVERINTERFACE_H
#include <unistd.h>
#include <map>

class PumpDriverInterface {
public:
    typedef struct{
      //the minimum possible flow in ml/s
      float minFlow;
      //the maximum possible flow in ml/s
      float maxFlow;
      //the flow precision in ml/s
      float flowPrecision;
    } PumpDefinition;

    virtual ~PumpDriverInterface(void){};

    virtual void init(const char* configTextPtr) = 0;

    virtual void deInit() = 0;

    virtual void getPumps(std::map<int,PumpDefinition>* pumpDefinitions) = 0;

    virtual void setPump(int pumpNumber, float flow) = 0;
};
#endif