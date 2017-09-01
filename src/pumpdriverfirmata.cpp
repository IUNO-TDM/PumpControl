#include "pumpdriverfirmata.h"
#include "easylogging++.h"

#include <thread>
#include <unistd.h>

using namespace std;

PumpDriverFirmata::PumpDriverFirmata() {
}

PumpDriverFirmata::~PumpDriverFirmata() {
}

bool PumpDriverFirmata::Init(const char *config_text_ptr, std::map<int, PumpDriverInterface::PumpDefinition> pump_definitions) {
    bool rv = false;
    std::vector<firmata::PortInfo> ports = firmata::FirmSerial::listPorts();
    pump_definitions_ = pump_definitions;
    for (auto i : pump_definitions_) {
        if (i.second.max_flow != i.second.min_flow) {
            pump_is_pwm_[i.first] = true;
        } else {
            pump_is_pwm_[i.first] = false;
        }
    }

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
        for (auto i : pump_definitions_) {
            if (pump_is_pwm_[i.first]) {
                firmata_->pinMode(i.second.output, MODE_PWM);
                // firmata_pinMode(firmata_, i.second, MODE_PWM);
            } else {
                firmata_->pinMode(i.second.output, MODE_OUTPUT);
                // firmata_pinMode(firmata_, i.second, MODE_OUTPUT);
            }
        }

        this_thread::sleep_for(chrono::seconds(2));
        for (auto i : pump_definitions_) {
            if (pump_is_pwm_[i.first]) {
                firmata_->analogWrite(i.second.output, 0);
                // firmata_analogWrite(firmata_, i.second, 0);
            } else {
                firmata_->digitalWrite(i.second.output, LOW);
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
        for (auto i : pump_definitions_) {
            if (pump_is_pwm_[i.first]) {
                firmata_->analogWrite(i.second.output, 0);
                // firmata_analogWrite(firmata_, i.second, 0);
            } else {
                firmata_->digitalWrite(i.second.output, LOW);
                // firmata_digitalWrite(firmata_, i.second, LOW);
            }
        }
    } catch (const std::exception& ex) {
        LOG(ERROR)<<"Exception on DeInit the PumpDriver: "<<ex.what();
    }
}

int PumpDriverFirmata::GetPumpCount() {
    return pump_definitions_.size();
}

float PumpDriverFirmata::SetFlow(int pump_number, float flow) {
    LOG(INFO)<< "setPump " << pump_number << " : "<< flow;
    float rv = 0;
    if(pump_is_pwm_[pump_number]) {
        if(flow < pump_definitions_[pump_number].min_flow) {
            firmata_->analogWrite(pump_definitions_[pump_number].output, 0);
            rv = 0;
        } else {
            int pwm_value = (flow - pump_definitions_[pump_number].min_flow) / (pump_definitions_[pump_number].max_flow - pump_definitions_[pump_number].min_flow) * 128 + 127;
            firmata_->analogWrite(pump_definitions_[pump_number].output, pwm_value);
            rv = flow;
        }
    } else {
        firmata_->digitalWrite(pump_definitions_[pump_number].output, (flow>0) ? HIGH : LOW);
        rv = (flow>0) ? pump_definitions_[pump_number].max_flow : 0;
    }
    return rv;
}

