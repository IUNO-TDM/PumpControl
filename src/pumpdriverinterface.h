#ifndef PUMPDRIVERINTERFACE_H
#define PUMPDRIVERINTERFACE_H

#include <map>
#include <vector>

class PumpDriverInterface {
    public:

        virtual ~PumpDriverInterface() {
        }

        virtual bool Init(const char* config_text) = 0;

        virtual void DeInit() = 0;

        virtual int GetPumpCount() = 0;

        virtual void SetPumpCurrent(int pump_number, float rel_pump_current) = 0;
};

#endif
