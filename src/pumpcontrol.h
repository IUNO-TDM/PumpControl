#ifndef PUMPCONTROL_H
#define PUMPCONTROL_H

#include "pumpcontrolinterface.h"
#include "iodriverinterface.h"
#include "timeprogramrunner.h"
#ifndef NO_ENCRYPTION
#include "cryptobuffer.h"
#endif

#include "json.hpp"

#include <map>
#include <vector>
#include <mutex>
#include <boost/bimap.hpp>

class PumpControl: public PumpControlInterface, public TimeProgramRunnerCallback, public IoDriverCallback {

    public:
        PumpControl(PumpDriverInterface* pump_driver,
                std::map<int, PumpDefinition> pump_configurations,
                IoDriverInterface* io_driver,
                float amount_override);

        virtual ~PumpControl();

        //PumpControlInterface
        virtual void RegisterCallbackClient(PumpControlCallback* client);
        virtual void UnregisterCallbackClient(PumpControlCallback* client);
        virtual void SetAmountForPump(int pump_number, int amount);
        virtual std::string GetIngredientForPump(int pump_number) const;
        virtual void SetIngredientForPump(int pump_number, const std::string& ingredient);
        virtual void DeleteIngredientForPump(int pump_number);
        virtual size_t GetNumberOfPumps() const;
        virtual PumpDefinition GetPumpDefinition(size_t pump_number) const;
        virtual float SwitchPump(size_t pump_number, bool switch_on);
        virtual void StartPumpTimed(size_t pump_number, float rel_current, float duration);
        virtual void StartProgram(unsigned long product_id, const std::string& receipt_json_string);
        virtual void EnterServiceMode();
        virtual void LeaveServiceMode();
        virtual void GetIoDesc(std::vector<IoDescription>& desc) const;
        virtual bool GetValue(const std::string& name) const;
        virtual void SetValue(const std::string& name, bool value);

        //TimeProgramRunnerCallback
        virtual void TimeProgramRunnerProgressUpdate(std::string id, int percent);
        virtual void TimeProgramRunnerStateUpdate(TimeProgramRunnerCallback::State state);
        virtual void TimeProgramRunnerProgramEnded(std::string id);
        virtual void SetFlow(size_t pump_number, float flow);
        virtual void SetAllPumpsOff();

        //IoDriverCallback
        virtual void NewInputState(const char* name, bool value);

        virtual void SetPumpControlState(PumpControlState state);

    private:

        struct FlowLog {
                float flow;
                std::chrono::system_clock::time_point start_time;
        };

#ifndef NO_ENCRYPTION
        static void DecryptProgram(unsigned long product_id, const std::string& in, CryptoBuffer& out);

        static void DecryptPrivate(const CryptoBuffer& in, CryptoBuffer& out);
#endif

        void PumpDriverAmountWarning(int pump_number, int amount_left);

        PumpControlState pumpcontrol_state_ = PUMP_STATE_UNINITIALIZED;
        std::recursive_mutex state_mutex_;
        PumpDriverInterface* pumpdriver_ = NULL;
        std::map<int, PumpDefinition> pump_definitions_;

        IoDriverInterface* io_driver_;

        PumpControlCallback* callback_client_ = NULL;
        std::recursive_mutex callback_client_mutex_;

        TimeProgramRunner* timeprogramrunner_ = NULL;
        TimeProgramRunner::TimeProgram timeprogram_;

        std::map<int, float> pump_amount_map_;
        std::map<int, FlowLog> flow_logs_;

        float amount_override_;

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

        void CheckIngredients(nlohmann::json j);
        int CreateTimeProgram(nlohmann::json j, TimeProgramRunner::TimeProgram &timeprogram);

        void TrackAmounts(int pump_number, float flow);

        int GetMaxElement(const std::map<int, float>& list);
        int GetMinElement(const std::map<int, float>& list);
        void SeparateTooFastIngredients(std::vector<int>& separated_pumps, std::map<int, float> min_list,
                std::map<int, float> max_list);

        const int warn_level = 100;
};

#endif
