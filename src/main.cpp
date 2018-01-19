#include "pumpcontrol.h"
#include "webinterface.h"
#include "pumpdriversimulation.h"
#include "pumpdriverfirmata.h"
#include "pumpdrivershield.h"

#include "easylogging++.h"

#include <boost/program_options.hpp>
#include <signal.h>
#include "licences.h"

using namespace std;
using namespace nlohmann;
using namespace boost::program_options;

INITIALIZE_EASYLOGGINGPP

bool sig_term_got = false;

void sigTermHandler( int ) {
    sig_term_got = true;
}

enum DriverType{
	SIMULATION,
#ifndef NO_REALDRIVERS
	FIRMATA,
	SHIELD
#endif
};

istream& operator>> (istream&in, DriverType& driver){
    string token;
    in >> token;

    if (token == "simulation"){
        driver = SIMULATION;
#ifndef NO_REALDRIVERS
    } else if (token == "firmata") {
        driver = FIRMATA;
    } else if (token == "shield") {
        driver = SHIELD;
#endif
    } else {
        throw validation_error (validation_error::invalid_option_value, "invalid driver type");
    }
    return in;
}

int main(int argc, char* argv[]) {

    LOG(INFO)<< "PumpControl starting up.";

    printf(licences);

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
        int tcp_port;
        string driver_config_string;
        string config_file;
        string homeDir = getenv("HOME");
        map<int, PumpControlInterface::PumpDefinition> pump_definitions;
        string std_conf_location = homeDir + "/pumpcontrol.settings.conf";
        {
            options_description general_options("General options");
            general_options.add_options()
                    ("version,v", "print version string")
                    ("help", "produce help message")
                    ("config,c", value<string>(&config_file)->default_value(std_conf_location), "name of a file of a configuration.");

            options_description config_options("Configuration options");
            config_options.add_options()
                    ("driver", value<DriverType>(&driver_type)->default_value(SIMULATION), "the driver to be used, one of [simulation|firmata|shield]")
                    ("driver-config", value<string>(&driver_config_string)->default_value(""), "a configuration string specific to the driver")
                    ("tcp-port", value<int>(&tcp_port)->default_value(9002), "the port the web interface is listening at");

            options_description pump_config("PumpConfiguration");
            string pump_config_str = "pump.";
            for (int i = 1; i <= 8; i++) {
                pump_definitions[i] = PumpControlInterface::PumpDefinition();
                pump_definitions[i].lookup_table.resize(10);
                for(size_t j=0; j<10; j++){
                    pump_config.add_options()
                            ((pump_config_str + to_string(i) + ".pwm_value." + to_string(j)).c_str(),
                                    value<float>(&pump_definitions[i].lookup_table[j].pwm_value)->default_value(NAN),
                                    (string("PWM value ") + to_string(j) + " in a range of 0.0 .. 1.0 for lookup table for pump "+ to_string(i)).c_str())
                            ((pump_config_str + to_string(i) + ".flow_value." + to_string(j)).c_str(),
                                    value<float>(&pump_definitions[i].lookup_table[j].flow)->default_value(NAN),
                                    (string("flow value ") + to_string(j) + " in l/s for lookup table for pump "+ to_string(i)).c_str());
               }
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

            if (vm.count("help")) {
                cout << visible << "\n";
                return 0;
            }

            if (vm.count("version")) {
                cout << "Multiple sources example, version 1.0\n";
                return 0;
            }

            ifstream ifs(config_file.c_str());
            if (!ifs) {
                LOG(ERROR)<< "Can not open config file: '" << config_file << "'.";
                return -1;
            }

            store(parse_config_file(ifs, config_file_options), vm);
            notify(vm);

            for(auto& pd: pump_definitions){
                pd.second.min_flow = NAN;
                pd.second.max_flow = NAN;
                bool erased = false;
                do{
                    erased = false;
                    for(vector<PumpControlInterface::LookupTableEntry>::iterator i = pd.second.lookup_table.begin(); i<pd.second.lookup_table.end(); i++){
                        if(isnan(i->pwm_value) || isnan(i->flow)){
                            pd.second.lookup_table.erase(i);
                            erased = true;
                            break;
                        }
                    }
                }while(erased);
                for(auto& l: pd.second.lookup_table){
                    if((l.flow > pd.second.max_flow) || isnan(pd.second.max_flow)){
                        pd.second.max_flow = l.flow;
                    }
                    if((l.flow < pd.second.min_flow) || isnan(pd.second.min_flow)){
                        pd.second.min_flow = l.flow;
                    }
                }
            }
        }

        {
            PumpDriverInterface* pump_driver = NULL;
            switch(driver_type){
                case SIMULATION:
                    LOG(INFO)<< "The simulation mode is set!";
                    pump_driver = new PumpDriverSimulation();
                    break;
#ifndef NO_REALDRIVERS
                    case FIRMATA:
                    pump_driver = new PumpDriverFirmata();
                     break;
                case SHIELD:
                    pump_driver = new PumpDriverShield();
                    break;
#endif
            }

            bool pump_driver_initialized = false;
            try{
                pump_driver_initialized= pump_driver->Init(driver_config_string.c_str());
                if(pump_driver_initialized) {
                    PumpControl pump_control(pump_driver, pump_definitions);
                    WebInterface web_interface(tcp_port, &pump_control);

                    while(!sig_term_got){
                        sleep(0xffffffff);
                    }

                }
            }catch(exception& e){
                LOG(ERROR) << "Caught exception: '" << e.what() << "'.";
            }catch(...){
                LOG(ERROR) << "Caught exception of unknown type, shutting down.";
            }

            if(pump_driver_initialized){
                pump_driver->DeInit();
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
