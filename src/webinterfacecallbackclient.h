#ifndef WEBINTERFACECALLBACKCLIENT_H
#define WEBINTERFACECALLBACKCLIENT_H

#include "pumpdriverinterface.h"
#include <string>
#include <unistd.h>

class WebInterfaceCallbackClient {
    public:
        enum PumpControlState {
            PUMP_STATE_UNINITIALIZED = 0,
            PUMP_STATE_IDLE = 1,
            PUMP_STATE_ACTIVE = 2,
            PUMP_STATE_SERVICE = 3,
            PUMP_STATE_ERROR = 4
        };

        static const char* NameForPumpControlState(PumpControlState state) {
            switch (state) {
                case PUMP_STATE_ACTIVE:
                    return "active";
                case PUMP_STATE_ERROR:
                    return "error";
                case PUMP_STATE_IDLE:
                    return "idle";
                case PUMP_STATE_SERVICE:
                    return "service";
                case PUMP_STATE_UNINITIALIZED:
                    return "uninitialized";
            }
            return "internal problem";
        }

        virtual ~WebInterfaceCallbackClient() {
        }

        virtual PumpControlState GetPumpControlState() const = 0;
        virtual void SetPumpControlState(PumpControlState state) = 0;
        virtual void SetAmountForPump(int pump_number, int amount) = 0;
        virtual std::string GetIngredientForPump(int pump_number) const = 0;
        virtual void SetIngredientForPump(int pump_number, const std::string& ingredient) = 0;
        virtual void DeleteIngredientForPump(int pump_number) = 0;
        virtual size_t GetNumberOfPumps() const = 0;
        virtual PumpDriverInterface::PumpDefinition GetPumpDefinition(size_t pump_index) const = 0;
        virtual float SwitchPump(size_t pump_index, bool switch_on) = 0;
        virtual void StartProgram(const char* receipt_json_string) = 0;
};

#endif
