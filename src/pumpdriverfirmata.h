#ifndef PUMPDRIVERFIRMATA_H
#define PUMPDRIVERFIRMATA_H
#include <pumpdriverinterface.h>
#include "firmata.h"

class PumpDriverFirmata: public PumpDriverInterface {
  public:
    PumpDriverFirmata();
    ~PumpDriverFirmata(void);

    void init(const char* configTextPtr);

    void deInit();

    void getPumps(std::map<int, PumpDefinition>* pumpDefinitions);

    void setPump(int pumpNumber, float flow);

  private:
    t_firmata     *mFirmata;
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

      //3 PWM
      //4 IO
      //5 PWM
      //6 PWM
      //7 IO
      //8 PWM
      //9 PWM
      //10 PWM
};
#endif