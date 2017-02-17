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
    const float MIN_FLOW = 0.0007;
    const float MAX_FLOW = 0.00143;
    const float FLOW_PRECISION = (MAX_FLOW-MIN_FLOW)/128;
    std::map<int, int> pump_to_output_{
      { 1, 3 },
      { 2, 4 },
      { 3, 5 },
      { 4, 6 },
      { 5, 7 },
      { 6, 8 },
      { 7, 9 },
      { 8, 10 }};

     std::map<int, bool> pump_is_pwm{
      { 1, true },
      { 2, false},
      { 3, true },
      { 4, true },
      { 5, false},
      { 6, false },
      { 7, true },
      { 8, true }}; 

      std::map<int,float> pumpFlows{
        {1 ,0.0},
        {2 ,0.0},
        {3 ,0.0},
        {4 ,0.0},
        {5 ,0.0},
        {6 ,0.0},
        {7 ,0.0},
        {8 ,0.0}
      } ;
};
#endif