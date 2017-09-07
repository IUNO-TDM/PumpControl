#include "pumpcontrol.h"
#include "webinterface.h"
#include "pumpdriversimulation.h"
#include "pumpdriverfirmata.h"
#include "pumpdrivershield.h"

#include "easylogging++.h"

#include <boost/program_options.hpp>
#include <signal.h>

using namespace std;
using namespace nlohmann;
using namespace boost::program_options;

INITIALIZE_EASYLOGGINGPP

bool sig_term_got = false;

void sigTermHandler( int ) {
    sig_term_got = true;
}

enum DriverType{ SIMULATION, FIRMATA, SHIELD };

istream& operator>> (istream&in, DriverType& driver){
    string token;
    in >> token;

    if (token == "simulation"){
        driver = SIMULATION;
    } else if (token == "firmata") {
        driver = FIRMATA;
    } else if (token == "shield") {
        driver = SHIELD;
    } else {
        throw validation_error (validation_error::invalid_option_value, "invalid driver type");
    }
    return in;
}

int main(int argc, char* argv[]) {

    LOG(INFO)<< "PumpControl starting up.";

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGTERM);
    sigdelset(&mask, SIGTSTP);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigTermHandler;
    struct sigaction so;
    memset(&so, 0, sizeof(so));

    bool sigterm_installed = (0 == sigaction(SIGTERM, &sa, &so));
    if(!sigterm_installed){
        LOG(WARNING) << "Could not install terminate handler.";
    }

    {
        DriverType driver_type;
        int websocket_port;
        string serial_port;
        string config_file;
        string homeDir = getenv("HOME");
        map<int, PumpDriverInterface::PumpDefinition> pump_definitions;
        string std_conf_location = homeDir + "/pumpcontrol.settings.conf";
        {
            options_description general_options("General options");
            general_options.add_options()
                    ("version,v", "print version string")
                    ("help", "produce help message")
                    ("config,c", value<string>(&config_file)->default_value(std_conf_location), "name of a file of a configuration.");

            options_description config_options("Configuration options");
            config_options.add_options()
                    ("driver", value<DriverType>(&driver_type)->default_value(SIMULATION), "the driver to be used, can be [simulation|firmata|shield]")
                    ("serialPort", value<string>(&serial_port)->default_value("/dev/tty.usbserial-A104WO1O"), "the full serial Port path")
                    ("webSocketPort", value<int>(&websocket_port)->default_value(9002), "The port of the listening WebSocket");

            options_description pump_config("PumpConfiguration");
            string pump_config_str = "pump.configuration.";
            for (int i = 1; i <= 8; i++) {
                pump_definitions[i] = PumpDriverInterface::PumpDefinition();
                pump_config.add_options()((pump_config_str + to_string(i) + ".output").c_str(),
                        value<int>(&(pump_definitions[i].output))->default_value(i),
                        (string("Output Pin for pump number ") + to_string(i)).c_str())(
                        (pump_config_str + to_string(i) + ".flow_precision").c_str(),
                        value<float>(&(pump_definitions[i].flow_precision))->default_value((0.00143 - 0.0007) / 128),
                        (string("Flow Precision for pump number ") + to_string(i)).c_str())(
                        (pump_config_str + to_string(i) + ".min_flow").c_str(),
                        value<float>(&(pump_definitions[i].min_flow))->default_value(0.0007),
                        (string("Min Flow for pump number ") + to_string(i)).c_str())(
                        (pump_config_str + to_string(i) + ".max_flow").c_str(),
                        value<float>(&(pump_definitions[i].max_flow))->default_value(0.00143),
                        (string("Max Flow for pump number ") + to_string(i)).c_str());
            }

            options_description cmdline_options;
            cmdline_options.add(general_options).add(config_options);

            options_description config_file_options;
            config_file_options.add(config_options).add(pump_config);

            options_description visible("Allowed options");
            visible.add(general_options).add(config_options);
            variables_map vm;
            store(command_line_parser(argc, argv).options(cmdline_options).run(), vm);
            notify(vm);

            ifstream ifs(config_file.c_str());
            if (!ifs) {
                LOG(INFO)<< "Can not open config file: " << config_file;
            }
            else
            {
                store(parse_config_file(ifs, config_file_options), vm);
                notify(vm);
            }

            if (vm.count("help")) {
                cout << visible << "\n";
                return 0;
            }

            if (vm.count("version")) {
                cout << "Multiple sources example, version 1.0\n";
                return 0;
            }
        }

        {
            string config_string;
            PumpDriverInterface* pump_driver = NULL;
            switch(driver_type){
                case SIMULATION:
                    LOG(INFO)<< "The simulation mode is set!";
                    pump_driver = new PumpDriverSimulation();
                    config_string = "simulation";
                    break;
                case FIRMATA:
                    pump_driver = new PumpDriverFirmata();
                    config_string = serial_port;
                    break;
                case SHIELD:
                    pump_driver = new PumpDriverShield();
                    break;
            }

            bool success = pump_driver->Init(config_string.c_str(), pump_definitions);
            if(success) {
                PumpControl pump_control(pump_driver, pump_definitions);
                WebInterface web_interface(websocket_port, &pump_control);

                while(!sig_term_got){
                    sleep(0xffffffff);
                }
            }

            delete pump_driver;
        }
    }

    if(sigterm_installed){
        sigaction(SIGTERM, &so, NULL);
    }

    LOG(INFO)<< "PumpControl exiting normally.";
    el::Loggers::flushAll();
    return 0;
}
