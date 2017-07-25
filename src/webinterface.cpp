#include "webinterface.h"

#include "easylogging++.h"
#include "websocketpp/common/thread.hpp"

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

using namespace std;

WebInterface::WebInterface(int port) {
    port_ = port;
}

WebInterface::~WebInterface() {
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebInterface::RegisterCallbackClient(WebInterfaceCallbackClient *client) {
    if( !callback_client_ ){
        callback_client_ = client;
    }else{
        throw logic_error("Tried to register a client where already one is registered");
    }
}

void WebInterface::UnregisterCallbackClient(WebInterfaceCallbackClient *client) {
    if( callback_client_ == client ){
        callback_client_ = NULL;
    }else{
        throw logic_error("Tried to unregister a client that isn't registered");
    }
}

void WebInterface::OnOpen(connection_hdl hdl) {
    {
        lock_guard<mutex> guard(connection_mutex_);
        LOG(DEBUG)<< "WebInterface onOpen";
        connections_.insert(hdl);
    }

    callback_client_->WebInterfaceOnOpen();
}

void WebInterface::OnClose(connection_hdl hdl) {
    lock_guard<mutex> guard(connection_mutex_);
    LOG(DEBUG)<< "WebInterface onClose";
    connections_.erase(hdl);
}

void WebInterface::OnHttp(connection_hdl hdl) {
    LOG(DEBUG)<< "WebInterface onHttp";
    WebSocketServer::connection_ptr con = server_.get_con_from_hdl(hdl);

    std::string body = con->get_request_body();
    std::string uri = con->get_resource();
    std::string met = con->get_request().get_method();
    WebInterfaceCallbackClient::HttpResponse response;

    callback_client_->WebInterfaceHttpMessage(met,uri,body, &response);

    con->set_body(response.response_message);
    con->set_status((websocketpp::http::status_code::value)response.response_code);
}

bool WebInterface::Start() {
    LOG(DEBUG)<< "Webinterface start on port " << port_;
    bool rv = false;
    try {
        server_.set_access_channels(websocketpp::log::alevel::all);
        server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
        server_.init_asio();
        server_.listen(port_);
        server_.set_open_handler(bind(&WebInterface::OnOpen,this, ::_1));
        server_.set_close_handler(bind(&WebInterface::OnClose,this,::_1));
        server_.set_http_handler(bind(&WebInterface::OnHttp,this,::_1));
        server_.start_accept();
        thread t([this] {
                    this->server_.run();
                });
        rv = true;
        server_thread_ = move(t);
    } catch(websocketpp::exception const & e) {
        LOG(ERROR)<<e.what();
    }
    return rv;
}

void WebInterface::Stop() {
    LOG(DEBUG)<< "Webinterface stop ";
    server_.stop();
    if(server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebInterface::SendMessage(std::string message) {
    lock_guard<mutex> guard(connection_mutex_);
    LOG(DEBUG)<< "WebInterface send message: " << message;

    for (auto con : connections_) {
        server_.send(con, message, websocketpp::frame::opcode::value::text);
    }
}
