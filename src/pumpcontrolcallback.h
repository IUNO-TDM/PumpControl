#ifndef PUMPCONTROLCALLBACK_H
#define PUMPCONTROLCALLBACK_H

#include "pumpcontrolinterface.h"

class PumpControlCallback {
    public:
        virtual ~PumpControlCallback(){
        }

        virtual void NewPumpControlState(PumpControlInterface::PumpControlState state) = 0;
        virtual void ProgressUpdate(std::string id, int percent) = 0;
        virtual void ProgramEnded(std::string id) = 0;
        virtual void AmountWarning(size_t pump_number, std::string ingredient, int warning_limit) = 0;
        virtual void Error(std::string error_type, int error_number, std::string details) = 0;
        virtual void NewInputState(const char* name, bool value) = 0;
};

#endif
