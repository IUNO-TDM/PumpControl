#include "pumpdriverfirmata.h"
#include "easylogging++.h"

#include <thread>
#include <unistd.h>

using namespace std;

const unsigned PumpDriverFirmata::pins_[] = {3,4,5,6,7,8,9,10};
const bool PumpDriverFirmata::pump_is_pwm_[] = {true, false, true, true, false, false, true, true};
const size_t PumpDriverFirmata::pump_count_ = sizeof(PumpDriverFirmata::pins_)/sizeof(PumpDriverFirmata::pins_[0]);

PumpDriverFirmata::PumpDriverFirmata() {
}

PumpDriverFirmata::~PumpDriverFirmata() {
}

bool PumpDriverFirmata::Init(const char *config_text_ptr) {
    bool rv = false;
    std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();

    LOG(INFO)<< "init with config text" << config_text_ptr;

    if (firmata_ != NULL) {
        delete firmata_;
        firmata_ = NULL;
    }

    try {
        serialio_ = new firmata::FirmSerial(config_text_ptr);
        if (serialio_->available()) {
            sleep(3); // Seems necessary on linux
            firmata_ = new firmata::Firmata<firmata::Base, firmata::I2C>(serialio_);
            firmata_->setSamplingInterval(100);
        }
        size_t pump_count = GetPumpCount();
        for (size_t i = 0; i < pump_count; i++) {
            unsigned pin = pins_[i];
            if (pump_is_pwm_[i]) {
                firmata_->pinMode(pin, MODE_PWM);
                // firmata_pinMode(firmata_, i.second, MODE_PWM);
            } else {
                firmata_->pinMode(pin, MODE_OUTPUT);
                // firmata_pinMode(firmata_, i.second, MODE_OUTPUT);
            }
        }

        this_thread::sleep_for(chrono::seconds(2));
        for (size_t i = 0; i < pump_count; i++) {
            unsigned pin = pins_[i];
            if (pump_is_pwm_[i]) {
                firmata_->analogWrite(pin, 0);
                // firmata_analogWrite(firmata_, i.second, 0);
            } else {
                firmata_->digitalWrite(pin, LOW);
                // firmata_digitalWrite(firmata_, i.second, LOW);
            }
        }
        rv = true;
    } catch (firmata::IOException& e) {
        LOG(ERROR)<< e.what();
    }
    catch(firmata::NotOpenException& e) {
        LOG(ERROR)<< e.what();
    }
    return rv;
}

void PumpDriverFirmata::DeInit() {
    try {
        size_t pump_count = GetPumpCount();
        for (size_t i = 0; i < pump_count; i++) {
            unsigned pin = pins_[i];
            if (pump_is_pwm_[i]) {
                firmata_->analogWrite(pin, 0);
                // firmata_analogWrite(firmata_, i.second, 0);
            } else {
                firmata_->digitalWrite(pin, LOW);
                // firmata_digitalWrite(firmata_, i.second, LOW);
            }
        }
    } catch (const std::exception& ex) {
        LOG(ERROR)<<"Exception on DeInit the PumpDriver: "<<ex.what();
    }
}

int PumpDriverFirmata::GetPumpCount() {
    return pump_count_;
}

void PumpDriverFirmata::SetPumpCurrent(int pump_number, float rel_pump_current) {
    LOG(INFO)<< "SetPumpCurrent " << pump_number << " : "<< rel_pump_current;
    unsigned pin = GetPinForPump(pump_number);
    if(IsPumpPwm(pump_number)) {
        unsigned pwm = static_cast<unsigned>(255.0 * rel_pump_current);
        firmata_->analogWrite(pin, pwm);
    } else {
        firmata_->digitalWrite(pin, (rel_pump_current>0) ? HIGH : LOW);
    }
}

unsigned PumpDriverFirmata::GetPinForPump(size_t pump_number){
    if(((pump_number-1) >= pump_count_) || (pump_number < 1)){
        throw out_of_range("pump number out of range");
    }
    return pins_[pump_number-1];
}

bool PumpDriverFirmata::IsPumpPwm(size_t pump_number){
    if(((pump_number-1) >= pump_count_) || (pump_number < 1)){
        throw out_of_range("pump number out of range");
    }
    return pump_is_pwm_[pump_number-1];
}

