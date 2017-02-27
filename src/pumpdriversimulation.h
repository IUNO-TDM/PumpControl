#ifndef PUMPDRIVERSIMULATION_H
#define PUMPDRIVERSIMULATION_H

#include <pumpdriverinterface.h>

class PumpDriverSimulation: public PumpDriverInterface {
  public:
    PumpDriverSimulation();
    ~PumpDriverSimulation();

    bool Init(const char* config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions);

    void DeInit();

    int GetPumpCount();
    void SetPump(int pump_number, float flow);

  private:

    std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions_;
    
};
#endif