#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "pumpcontrolcallback.h"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"

#include <thread>
#include <mutex>
#include <set>

class WebInterface : public PumpControlCallback{

    public:

        WebInterface(int port, PumpControlInterface* pump_control);
        virtual ~WebInterface();

        //PumpControlCallback
        virtual void NewPumpControlState(PumpControlInterface::PumpControlState state);
        virtual void ProgressUpdate(std::string id, int percent);
        virtual void ProgramEnded(std::string id);
        virtual void AmountWarning(size_t pump_number, std::string ingredient, int warning_limit);
        virtual void Error(std::string error_type, int error_number, std::string details);
        virtual void NewInputState(const char* name, bool value);

    private:

        struct HttpResponse {
                int response_code;
                std::string response_message;
                std::string content_type_;
                void Set(int code, const char* msg){
                    response_code=code;
                    response_message = msg;
                }
                void Set(int code, const std::string& msg){
                    response_code=code;
                    response_message = msg;
                }
                void SetContentType(const char* content_type){
                    content_type_ = content_type;
                }
        };

        void OnOpen(websocketpp::connection_hdl hdl);
        void OnClose(websocketpp::connection_hdl hdl);
        void OnHttp(websocketpp::connection_hdl hdl);

        void HandleHttpMessage(const std::string& method, const std::string& path, const std::string& body,
                HttpResponse& response);

        void HandleStartProgram(const std::string& product_id, const std::string& program_string, HttpResponse& response);
        void HandleGetPumps(HttpResponse& response);
        void HandleSetAmountForPump(const std::string& pump_number_string, const std::string& amount_string, HttpResponse& response);
        void HandleGetIngredientForPump(const std::string& pump_number_string, HttpResponse& response);
        void HandleSetIngredientForPump(const std::string& pump_number_string, const std::string& ingredient, HttpResponse& response);
        void HandleDeleteIngredientForPump(const std::string& pump_number_string, HttpResponse& response);
        void HandleEnterServiceMode(HttpResponse& response);
        void HandleLeaveServiceMode(HttpResponse& response);
        void HandleSwitchPump(const std::string& pump_number_string, const std::string& on_off, HttpResponse& response);
        void HandleStartPumpTimed(const std::string& pump_number_string, const std::string& current_string, const std::string& duration_string, HttpResponse& response);
        void HandleGetIoDesc(HttpResponse& response);
        void HandleGetValue(const std::string& name, HttpResponse& response);
        void HandleSetValue(const std::string& name, const std::string& value_str, HttpResponse& response);

        void SendMessage(const std::string& message);

        websocketpp::server<websocketpp::config::asio> server_;
        std::thread server_thread_;
        int port_;

        std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl> > connections_;
        std::recursive_mutex connection_mutex_;

        PumpControlInterface* pump_control_ = NULL;

        std::map<std::string, std::string> states_;
        std::recursive_mutex states_mutex_;
};

#endif
