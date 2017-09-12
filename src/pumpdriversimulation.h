#ifndef PUMPDRIVERSIMULATION_H
#define PUMPDRIVERSIMULATION_H

#include <pumpdriverinterface.h>

class PumpDriverSimulation: public PumpDriverInterface {
    public:
        PumpDriverSimulation();
        virtual ~PumpDriverSimulation();

        virtual bool Init(const char* config_text);
        virtual void DeInit();

        virtual int GetPumpCount();
        virtual void SetPumpCurrent(int pump_number, float rel_pump_current);
};

#endif
