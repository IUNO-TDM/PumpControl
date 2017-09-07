#ifndef SRC_PUMPDRIVERSHIELD_H_
#define SRC_PUMPDRIVERSHIELD_H_

#include "pumpdriverinterface.h"

class PumpDriverShield: public PumpDriverInterface {
    public:
        PumpDriverShield();
        virtual ~PumpDriverShield();

        virtual bool Init(const char* config_text, const std::map<int, PumpDefinition>& pump_definitions);
        virtual void DeInit();

        virtual int GetPumpCount();
        virtual float SetFlow(int pump_number, float flow);

    private:
        static unsigned GetPinForPump(size_t pump_number);

        std::map<int, PumpDefinition> pump_definitions_;
        bool initialized_ = false;

        static const unsigned pins_[];
        static const size_t pump_count_;
};

#endif
