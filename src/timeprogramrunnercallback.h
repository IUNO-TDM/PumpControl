#ifndef TIMEPROGRAMRUNNERCALLBACK_H_
#define TIMEPROGRAMRUNNERCALLBACK_H_

#include "timeprogramrunner.h"

class TimeProgramRunnerCallback {
    public:
        virtual ~TimeProgramRunnerCallback() {
        }

        virtual void TimeProgramRunnerProgressUpdate(std::string id, int percent) = 0;
        virtual void TimeProgramRunnerStateUpdate(TimeProgramRunner::TimeProgramRunnerState state) = 0;
        virtual void TimeProgramRunnerProgramEnded(std::string id) = 0;
};

#endif
