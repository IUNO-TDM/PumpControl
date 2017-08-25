#ifndef PUMPCONTROL_H
#define PUMPCONTROL_H

#include "pumpcontrolinterface.h"
#include "timeprogramrunner.h"
#include "cryptobuffer.h"

#include "json.hpp"

#include <map>
#include <vector>
#include <mutex>
#include <boost/bimap.hpp>

class PumpControl: public PumpControlInterface, public TimeProgramRunnerCallback {

    public:
        PumpControl(PumpDriverInterface* pump_driver,
                std::map<int, PumpDriverInterface::PumpDefinition> pump_configurations);

        virtual ~PumpControl();

        //PumpControlInterface
        virtual void RegisterCallbackClient(PumpControlCallback* client);
        virtual void UnregisterCallbackClient(PumpControlCallback* client);
        virtual void SetAmountForPump(int pump_number, int amount);
        virtual std::string GetIngredientForPump(int pump_number) const;
        virtual void SetIngredientForPump(int pump_number, const std::string& ingredient);
        virtual void DeleteIngredientForPump(int pump_number);
        virtual size_t GetNumberOfPumps() const;
        virtual PumpDriverInterface::PumpDefinition GetPumpDefinition(size_t pump_number) const;
        virtual float SwitchPump(size_t pump_number, bool switch_on);
        virtual void StartProgram(const std::string& receipt_json_string);
        virtual void EnterServiceMode();
        virtual void LeaveServiceMode();

        //TimeProgramRunnerCallback
        virtual void TimeProgramRunnerProgressUpdate(std::string id, int percent);
        virtual void TimeProgramRunnerStateUpdate(TimeProgramRunnerCallback::State state);
        virtual void TimeProgramRunnerProgramEnded(std::string id);
        virtual void SetFlow(size_t pump_number, float flow);
        virtual void SetAllPumpsOff();

        virtual void SetPumpControlState(PumpControlState state);

    private:

        struct FlowLog {
                float flow;
                std::chrono::system_clock::time_point start_time;
        };

        static void DecryptProgram(const std::string& in, std::string& out);

        static void DecryptPrivate(const CryptoBuffer& in, CryptoBuffer& out);

        void PumpDriverAmountWarning(int pump_number, int amount_left);

        PumpControlState pumpcontrol_state_ = PUMP_STATE_UNINITIALIZED;
        PumpDriverInterface* pumpdriver_ = NULL;
        std::map<int, PumpDriverInterface::PumpDefinition> pump_definitions_;

        PumpControlCallback* callback_client_ = NULL;
        std::mutex callback_client_mutex_;

        TimeProgramRunner* timeprogramrunner_ = NULL;
        TimeProgramRunner::TimeProgram timeprogram_;

        std::map<int, float> pump_amount_map_;
        std::map<int, FlowLog> flow_logs_;

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

        int CreateTimeProgram(nlohmann::json j, TimeProgramRunner::TimeProgram &timeprogram);

        void TrackAmounts(int pump_number, float flow);

        int GetMaxElement(std::map<int, float> list);
        int GetMinElement(std::map<int, float> list);
        void SeparateTooFastIngredients(std::vector<int> *separated_pumps, std::map<int, float> min_list,
                std::map<int, float> max_list);

        const int warn_level = 100;
};

#endif
