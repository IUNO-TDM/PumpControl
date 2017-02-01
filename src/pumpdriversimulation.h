#ifndef PUMPDRIVERSIMULATION_H
#define PUMPDRIVERSIMULATION_H

#include <pumpdriverinterface.h>

class PumpDriverSimulation: public PumpDriverInterface {
  public:
    PumpDriverSimulation();
    ~PumpDriverSimulation(void);

    void init(const char* configTextPtr);

    void deInit();

    void getPumps(std::map<int, PumpDefinition>* pumpDefinitions);

    void setPump(int pumpNumber, float flow);

  private:
    std::map<int, int> mPumpToOutput{
        { 1, 3 },
        { 2, 4 },
        { 3, 5 },
        { 4, 6 },
        { 5, 7 },
        { 6, 8 },
        { 7, 9 },
        { 8, 10 }};
      
      const float FLOW = 1.43;
};
#endif