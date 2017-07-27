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
        };

        void OnOpen(websocketpp::connection_hdl hdl);
        void OnClose(websocketpp::connection_hdl hdl);
        void OnHttp(websocketpp::connection_hdl hdl);

        bool WebInterfaceHttpMessage(std::string method, std::string path, std::string body,
                HttpResponse *response);

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
