#ifndef PUMPDRIVERFIRMATA_H
#define PUMPDRIVERFIRMATA_H

#include "pumpdriverinterface.h"
#include "firmata.h"
#include "firmserial.h"

class PumpDriverFirmata: public PumpDriverInterface {
    public:
        PumpDriverFirmata();
        virtual ~PumpDriverFirmata();

        virtual bool Init(const char* config_text);
        virtual void DeInit();

        virtual int GetPumpCount();
        virtual void SetPumpCurrent(int pump_number, float rel_pump_current);

    private:
        static unsigned GetPinForPump(size_t pump_number);
        static bool IsPumpPwm(size_t pump_number);

        firmata::Firmata<firmata::Base, firmata::I2C>* firmata_ = NULL;
        firmata::FirmSerial* serialio_ = NULL;


        static const unsigned pins_[];
        static const bool pump_is_pwm_[];
        static const size_t pump_count_;
};

#endif
