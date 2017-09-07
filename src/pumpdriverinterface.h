#ifndef PUMPDRIVERINTERFACE_H
#define PUMPDRIVERINTERFACE_H

#include <map>
#include <vector>

class PumpDriverInterface {
    public:

        static const size_t lookup_table_entry_count = 10;

        struct LookupTableEntry {
            float pwm_value;
            float flow;
        };

        struct PumpDefinition {
                //the minimum possible flow in ml/s
                float min_flow;
                //the maximum possible flow in ml/s
                float max_flow;

                std::vector<LookupTableEntry> lookup_table;
        };

        virtual ~PumpDriverInterface() {
        }

        virtual bool Init(const char* config_text, const std::map<int, PumpDefinition>& pump_definitions) = 0;

        virtual void DeInit() = 0;

        virtual int GetPumpCount() = 0;

        virtual float SetFlow(int pump_number, float flow) = 0;
};

#endif
