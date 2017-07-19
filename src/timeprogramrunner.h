#include "pumpdriverinterface.h"
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stdlib.h>
#include <string>

class TimeProgramRunnerCallback;
class TimeProgramRunner {
    public:
        typedef std::map<int, std::map<int,float> > TimeProgram;

        enum TimeProgramRunnerState{
            TIME_PROGRAM_INIT = 0,
            TIME_PROGRAM_IDLE = 1,
            TIME_PROGRAM_ACTIVE = 2,
            TIME_PROGRAM_STOPPING = 3
        };
        
        TimeProgramRunner(TimeProgramRunnerCallback *callback_client, PumpDriverInterface *pump_driver);
        virtual ~TimeProgramRunner();

        void Run();
        void Shutdown();
        void StartProgram(std::string id, TimeProgram time_program);
        void EmergencyStopProgram();
        static const char* NameForState(TimeProgramRunnerState state);

    private:
        TimeProgramRunnerCallback *callback_client_;
        PumpDriverInterface *pump_driver_;
        TimeProgramRunnerState timeprogramrunner_target_state_ = TIME_PROGRAM_IDLE;
        TimeProgramRunnerState timeprogramrunner_state_ = TIME_PROGRAM_INIT;
        TimeProgram timeprogram_;
        std::condition_variable condition_variable_;
        std::mutex time_lock_mutex_;
        std::mutex state_machine_mutex_;
        // std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions_;
        std::string programm_id_;
};

class TimeProgramRunnerCallback{
    public:
		virtual ~TimeProgramRunnerCallback(){}

        virtual void TimeProgramRunnerProgressUpdate(std::string id, int percent) = 0;
        virtual void TimeProgramRunnerStateUpdate(TimeProgramRunner::TimeProgramRunnerState state) = 0;
        virtual void TimeProgramRunnerProgramEnded(std::string id ) = 0;
};
