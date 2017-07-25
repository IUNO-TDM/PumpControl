#ifndef PUMPDRIVERFIRMATA_H
#define PUMPDRIVERFIRMATA_H

#include "pumpdriverinterface.h"
#include "firmata.h"
#include "firmserial.h"

#include <chrono>

class PumpDriverFirmata: public PumpDriverInterface {
    public:
        PumpDriverFirmata();
        virtual ~PumpDriverFirmata();

        bool Init(const char* config_text_ptr, std::map<int, PumpDriverInterface::PumpDefinition> pump_definitions,
                PumpDriverCallbackClient* callbackClient);
        void DeInit();

        // void GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions);
        int GetPumpCount();
        void SetPump(int pump_number, float flow);
        void SetAmountForPump(int pump_number, int amount);

        struct FlowLog {
                float flow;
                std::chrono::system_clock::time_point start_time;
        };

    private:
        // t_firmata     *firmata_;
        firmata::Firmata<firmata::Base, firmata::I2C>* firmata_ = NULL;
        firmata::FirmSerial* serialio_ = NULL;

        std::map<int, PumpDriverInterface::PumpDefinition> pump_definitions_;

        std::map<int, bool> pump_is_pwm_;

        std::map<int, float> pump_amount_map_;

        std::map<int, FlowLog> flow_logs_;

        const int warn_level = 100;
        PumpDriverCallbackClient* callback_client_ = NULL;

};

#endif
