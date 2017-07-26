#ifndef PUMPCONTROL_H
#define PUMPCONTROL_H

#include "json.hpp"
#include "webinterface.h"
#include "timeprogramrunner.h"

#include <unistd.h>
#include <map>
#include <vector>
#include <thread>
#include <boost/bimap.hpp>

class PumpControl: public PumpControlInterface, public TimeProgramRunnerCallback, public PumpDriverCallbackClient {

    public:
        PumpControl(std::string serial_port, bool simulation, int websocket_port,
                std::map<int, PumpDriverInterface::PumpDefinition> pump_configurations);

        virtual ~PumpControl();

        //PumpControlInterface
        virtual PumpControlState GetPumpControlState() const;
        virtual void SetPumpControlState(PumpControlState state);
        virtual void SetAmountForPump(int pump_number, int amount);
        virtual std::string GetIngredientForPump(int pump_number) const;
        virtual void SetIngredientForPump(int pump_number, const std::string& ingredient);
        virtual void DeleteIngredientForPump(int pump_number);
        virtual size_t GetNumberOfPumps() const;
        virtual PumpDriverInterface::PumpDefinition GetPumpDefinition(size_t pump_index) const;
        virtual float SwitchPump(size_t pump_index, bool switch_on);
        virtual void StartProgram(const char* receipt_json_string);

        //TimeProgramRunnerCallback
        virtual void TimeProgramRunnerProgressUpdate(std::string id, int percent);
        virtual void TimeProgramRunnerStateUpdate(TimeProgramRunnerCallback::State state);
        virtual void TimeProgramRunnerProgramEnded(std::string id);
        virtual void PumpDriverAmountWarning(int pump_number, int amount_left);

    private:
        PumpControlState pumpcontrol_state_ = PUMP_STATE_UNINITIALIZED;
        PumpDriverInterface *pumpdriver_ = NULL;
        std::map<int, PumpDriverInterface::PumpDefinition> pump_definitions_;

        WebInterface *webinterface_ = NULL;
        TimeProgramRunner *timeprogramrunner_ = NULL;

        TimeProgramRunner::TimeProgram timeprogram_;

        std::string serialport_;
        bool simulation_;

        boost::bimap<int, std::string> pump_ingredients_bimap_;
        
        const std::map<int, std::string> kPumpIngredientsInit {
                { 1, "Orangensaft" },
                { 2, "Apfelsaft" },
                { 3, "Sprudel" },
                { 4, "Kirschsaft" },
                { 5, "Bananensaft" },
                { 6, "Johannisbeersaft" },
                { 7, "Cola" },
                { 8, "Fanta" } };

        std::thread timeprogramrunner_thread_;

        int CreateTimeProgram(nlohmann::json j, TimeProgramRunner::TimeProgram &timeprogram);

        void TimerWorker(int interval, int maximumTime);
        void CreateTimer(int interval, int maximumTime);
        void TimerFired(int time);
        void TimerEnded();
        void Init(std::string serial_port, bool simulation, int websocket_port,
                std::map<int, PumpDriverInterface::PumpDefinition> pump_configurations);
        const char *NameForPumpControlState(PumpControlState state);
        int GetMaxElement(std::map<int, float> list);
        int GetMinElement(std::map<int, float> list);
        void SeparateTooFastIngredients(std::vector<int> *separated_pumps, std::map<int, float> min_list,
                std::map<int, float> max_list);
};

#endif
