#ifndef PUMPDRIVERFIRMATA_H
#define PUMPDRIVERFIRMATA_H

#include "pumpdriverinterface.h"
#include "firmata.h"
#include "firmserial.h"

class PumpDriverFirmata: public PumpDriverInterface {
    public:
        PumpDriverFirmata();
        virtual ~PumpDriverFirmata();

        virtual bool Init(const char* config_text, const std::map<int, PumpDefinition>& pump_definitions);
        virtual void DeInit();

        virtual int GetPumpCount();
        virtual float SetFlow(int pump_number, float flow);

    private:
        static unsigned GetPinForPump(size_t pump_number);

        firmata::Firmata<firmata::Base, firmata::I2C>* firmata_ = NULL;
        firmata::FirmSerial* serialio_ = NULL;

        std::map<int, PumpDefinition> pump_definitions_;
        std::map<int, bool> pump_is_pwm_;

        static const unsigned pins_[];
        static const size_t pump_count_;
};

#endif
