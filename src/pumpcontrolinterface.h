#ifndef PUMPCONTROLINTERFACE_H
#define PUMPCONTROLINTERFACE_H

#include "pumpdriverinterface.h"
#include <string>
#include <unistd.h>

class PumpControlCallback;

class PumpControlInterface {
    public:
        class not_in_this_state : public std::logic_error {
            public:
              explicit not_in_this_state (const std::string& what_arg) : std::logic_error(what_arg){}
        };

        enum PumpControlState {
            PUMP_STATE_UNINITIALIZED = 0,
            PUMP_STATE_IDLE = 1,
            PUMP_STATE_ACTIVE = 2,
            PUMP_STATE_SERVICE = 3,
            PUMP_STATE_ERROR = 4
        };

        enum Timing {
            TIMING_BY_MACHINE = 0,
            TIMING_ALL_FAST_START_PARALLEL = 1,
            TIMING_INTERPOLATED_FINISH_PARALLEL = 2,
            TIMING_SEQUENTIAL = 3
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

        virtual ~PumpControlInterface() {
        }

        virtual void RegisterCallbackClient(PumpControlCallback* client) = 0;
        virtual void UnregisterCallbackClient(PumpControlCallback* client) = 0;
        virtual void SetAmountForPump(int pump_number, int amount) = 0;
        virtual std::string GetIngredientForPump(int pump_number) const = 0;
        virtual void SetIngredientForPump(int pump_number, const std::string& ingredient) = 0;
        virtual void DeleteIngredientForPump(int pump_number) = 0;
        virtual size_t GetNumberOfPumps() const = 0;
        virtual PumpDriverInterface::PumpDefinition GetPumpDefinition(size_t pump_number) const = 0;
        virtual float SwitchPump(size_t pump_number, bool switch_on) = 0;
        virtual void StartProgram(const std::string& receipt_json_string) = 0;
        virtual void EnterServiceMode() = 0;
        virtual void LeaveServiceMode() = 0;
};

#endif
