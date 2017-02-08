#include "pumpdriverinterface.h"
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class TimeProgramRunnerCallback;
class TimeProgramRunner {
    public:
        typedef std::map<int,float> TimeCommand;
        typedef std::map<int,TimeCommand> TimeProgram;

        typedef enum {
            TIME_PROGRAM_INIT = 0,
            TIME_PROGRAM_IDLE = 1,
            TIME_PROGRAM_ACTIVE = 2,
            TIME_PROGRAM_STOPPING = 3
        }TimeProgramRunnerState;    
        
        TimeProgramRunner(TimeProgramRunnerCallback *callback_client, PumpDriverInterface *pump_driver);
        ~TimeProgramRunner();
        void Run();
        void Shutdown();
        void StartProgram(const char* id, TimeProgram time_program);
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
        std::map<int,PumpDriverInterface::PumpDefinition> pumpdefinitions_;
        const char* programm_id_;
};

class TimeProgramRunnerCallback{
    public:
        virtual void TimeProgramRunnerProgressUpdate(const char* id, int percent) = 0;
        virtual void TimeProgramRunnerStateUpdate(TimeProgramRunner::TimeProgramRunnerState state) = 0;
        virtual void TimeProgramRunnerProgramEnded(const char* id ) = 0;
};