#ifndef TIMEPROGRAMRUNNER_H
#define TIMEPROGRAMRUNNER_H

#include "timeprogramrunnercallback.h"

#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>

class TimeProgramRunner {
    public:
        typedef std::map<int, std::map<int, float> > TimeProgram;

        TimeProgramRunner(TimeProgramRunnerCallback *callback_client);
        virtual ~TimeProgramRunner();

        void Run();
        void Shutdown();
        void StartProgram(std::string id, TimeProgram time_program);
        void EmergencyStopProgram();

    private:
        TimeProgramRunnerCallback* callback_client_;
        TimeProgramRunnerCallback::State timeprogramrunner_target_state_ =
                TimeProgramRunnerCallback::TIME_PROGRAM_IDLE;
        TimeProgramRunnerCallback::State timeprogramrunner_state_ =
                TimeProgramRunnerCallback::TIME_PROGRAM_INIT;
        TimeProgram timeprogram_;
        std::condition_variable condition_variable_;
        std::mutex time_lock_mutex_;
        std::mutex state_machine_mutex_;
        std::string programm_id_;
        std::thread thread_;
};

#endif
