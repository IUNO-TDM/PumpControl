#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "webinterfacecallbackclient.h"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"

#include <map>
#include <set>
#include <unistd.h>

typedef std::map<std::string, WebInterfaceCallbackClient*> CallbackClientsMap;
typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;

class WebInterface {
    public:
        WebInterface(int port);
        virtual ~WebInterface();

        void RegisterCallbackClient(WebInterfaceCallbackClient *);
        void UnregisterCallbackClient(WebInterfaceCallbackClient *);
        void Init(int port);
        bool Start();
        void Stop();
        void SendMessage(std::string message);

    private:
        CallbackClientsMap callback_clients_;
        int port_;
        WebSocketServer server_;
        std::thread server_thread_;
        void OnOpen(websocketpp::connection_hdl hdl);

        void OnClose(websocketpp::connection_hdl hdl);

        void OnHttp(websocketpp::connection_hdl hdl);

        typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl> > ConList;
        ConList connections_;

        websocketpp::lib::mutex connection_mutex_;
};

#endif
