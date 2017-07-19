#ifndef PUMPDRIVERSIMULATION_H
#define PUMPDRIVERSIMULATION_H

#include <pumpdriverinterface.h>

#include <chrono>

class PumpDriverSimulation: public PumpDriverInterface {
  public:
    PumpDriverSimulation();
    virtual ~PumpDriverSimulation();

    bool Init(const char* config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions, PumpDriverCallbackClient* callbackClient);

    void DeInit();

    int GetPumpCount();
    void SetPump(int pump_number, float flow);
    void SetAmountForPump(int pump_number, int amount);

    struct FlowLog{
      float flow;
      std::chrono::system_clock::time_point start_time;
	};

  private:

    std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions_;
    std::map<int,float> pump_amount_map_;
    std::map<int,FlowLog> flow_logs_;

    const int warn_level = 100;

    PumpDriverCallbackClient* callback_client_;
};

#endif
