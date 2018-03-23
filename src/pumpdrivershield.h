#ifndef SRC_PUMPDRIVERSHIELD_H_
#define SRC_PUMPDRIVERSHIELD_H_

#include "pumpdriverinterface.h"

class PumpDriverShield: public PumpDriverInterface {
    public:
        PumpDriverShield();
        virtual ~PumpDriverShield();

        virtual bool Init(const char* config_text);
        virtual void DeInit();

        virtual int GetPumpCount();
        virtual void SetPumpCurrent(int pump_number, float rel_pump_current);

    private:
        static unsigned GetPinForPump(size_t pump_number);
        static void ParseConfigString(const char* config_text);

        bool initialized_ = false;

        static unsigned pins_[];
        static const size_t pump_count_;
};

#endif
