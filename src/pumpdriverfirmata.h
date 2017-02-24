#ifndef PUMPDRIVERFIRMATA_H
#define PUMPDRIVERFIRMATA_H
#include <pumpdriverinterface.h>
#include "firmata.h"
#include "firmserial.h"

class PumpDriverFirmata: public PumpDriverInterface {
  public:
    PumpDriverFirmata();
    ~PumpDriverFirmata(void);

    void Init(const char* config_text_ptr, std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions);

    void DeInit();

    // void GetPumps(std::map<int, PumpDriverInterface::PumpDefinition>* pump_definitions);
    int GetPumpCount();
    void SetPump(int pump_number, float flow);

  private:
    // t_firmata     *firmata_;
    firmata::Firmata<firmata::Base, firmata::I2C>* firmata_ = NULL;
	  firmata::FirmSerial* serialio_;

    std::map<int,PumpDriverInterface::PumpDefinition> pump_definitions_;


     std::map<int, bool> pump_is_pwm_;

};
#endif