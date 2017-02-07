#ifndef PUMPDRIVERSIMULATION_H
#define PUMPDRIVERSIMULATION_H

#include <pumpdriverinterface.h>

class PumpDriverSimulation: public PumpDriverInterface {
  public:
    PumpDriverSimulation();
    ~PumpDriverSimulation(void);

    void Init(const char* config_text_ptr);

    void DeInit();

    void GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions);

    void SetPump(int pump_number, float flow);

  private:
    std::map<int, int> pump_to_output_{
        { 1, 3 },
        { 2, 4 },
        { 3, 5 },
        { 4, 6 },
        { 5, 7 },
        { 6, 8 },
        { 7, 9 },
        { 8, 10 }};
      
      const float FLOW = 0.00143;
};
#endif