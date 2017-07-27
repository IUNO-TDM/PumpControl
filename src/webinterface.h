#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "pumpcontrolcallback.h"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"

#include <map>
#include <set>
#include <unistd.h>

class WebInterface : public PumpControlCallback{

    public:

        WebInterface(int port, PumpControlInterface* pump_control);
        virtual ~WebInterface();

        //PumpControlCallback
        virtual void NewPumpControlState(PumpControlInterface::PumpControlState state);
        virtual void ProgramEnded(std::string id);
        virtual void ProgressUpdate(std::string id, int percent);
        virtual void AmountWarning(size_t pump_number, std::string ingredient, int warning_limit);
        virtual void Error(std::string error_type, int error_number, std::string details);

        bool Start();
        void Stop();

    private:

        struct HttpResponse {
                int response_code;
                std::string response_message;
                void Set(int code, const char* msg){
                    response_code=code;
                    response_message = msg;
                }
                void Set(int code, const std::string& msg){
                    response_code=code;
                    response_message = msg;
                }
        };

        void OnOpen(websocketpp::connection_hdl hdl);
        void OnClose(websocketpp::connection_hdl hdl);
        void OnHttp(websocketpp::connection_hdl hdl);

        void HandleHttpMessage(const std::string& method, const std::string& path, const std::string& body,
                HttpResponse& response);

        void HandleSetAmountForPump(const std::string& pump_number_string, const std::string& amount_string, HttpResponse& response);
        void HandleSetIngredientForPump(const std::string& pump_number_string, const std::string& ingredient, HttpResponse& response);
        void HandleGetIngredientForPump(const std::string& pump_number_string, HttpResponse& response);
        void HandleDeleteIngredientForPump(const std::string& pump_number_string, HttpResponse& response);
        void HandleGetPumps(HttpResponse& response);
        void HandleSwitchPump(const std::string& pump_number_string, const std::string& on_off, HttpResponse& response);
        void HandleEnterServiceMode(HttpResponse& response);
        void HandleLeaveServiceMode(HttpResponse& response);
        void HandleStartProgram(const std::string& program_string, HttpResponse& response);

        void SendMessage(std::string message);

        typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
        WebSocketServer server_;
        std::thread server_thread_;
        int port_;

        typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl> > ConList;
        ConList connections_;
        websocketpp::lib::mutex connection_mutex_;

        PumpControlInterface* pump_control_ = NULL;
        PumpControlInterface::PumpControlState pump_control_state_ = PumpControlInterface::PUMP_STATE_UNINITIALIZED;
};

#endif
