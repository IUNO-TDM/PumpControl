#ifndef TIMEPROGRAMRUNNERCALLBACK_H_
#define TIMEPROGRAMRUNNERCALLBACK_H_

#include <string>

class TimeProgramRunnerCallback {
    public:
        enum State {
            TIME_PROGRAM_INIT = 0,
            TIME_PROGRAM_IDLE = 1,
            TIME_PROGRAM_ACTIVE = 2,
            TIME_PROGRAM_STOPPING = 3
        };

        static const char* NameForState(TimeProgramRunnerCallback::State state) {
            switch (state) {
                case TIME_PROGRAM_ACTIVE:
                    return "active";
                case TIME_PROGRAM_INIT:
                    return "init";
                case TIME_PROGRAM_IDLE:
                    return "idle";
                case TIME_PROGRAM_STOPPING:
                    return "stopping";
            }
            return "internal problem";
        }

        virtual ~TimeProgramRunnerCallback() {
        }

        virtual void TimeProgramRunnerProgressUpdate(std::string id, int percent) = 0;
        virtual void TimeProgramRunnerStateUpdate(State state) = 0;
        virtual void TimeProgramRunnerProgramEnded(std::string id) = 0;
};

#endif
