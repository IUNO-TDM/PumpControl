#include <webinterface.h>
#include <websocketpp/common/thread.hpp>
#include "easylogging++.h"
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

WebInterface::WebInterface(int port){
  port_ = port;
  // callback_clients_ = new callback_clients_Map();
  

}

WebInterface::~WebInterface(){
  if(server_thread_.joinable()){
    server_thread_.join();
  }
}

void WebInterface::RegisterCallbackClient(WebInterfaceCallbackClient *client){
  callback_clients_[client->GetClientName()] = client;
}
void WebInterface::UnregisterCallbackClient(WebInterfaceCallbackClient *client){
  callback_clients_.erase(client->GetClientName());
}

void WebInterface::OnOpen(connection_hdl hdl){
  {
    lock_guard<mutex> guard(connection_mutex_);
    LOG(DEBUG) << "WebInterface onOpen";
    connections_.insert(hdl);
  }  
  for(CallbackClientsMap::iterator i = callback_clients_.begin();
      i!= callback_clients_.end(); i++)
  {
    i->second->WebInterfaceOnOpen();
  }
  
}

void WebInterface::OnClose(connection_hdl hdl){
  lock_guard<mutex> guard(connection_mutex_);
  LOG(DEBUG) << "WebInterface onClose";
  connections_.erase(hdl);
}
void WebInterface::OnHttp(connection_hdl hdl){
  LOG(DEBUG) << "WebInterface onHttp";
  WebSocketServer::connection_ptr con = server_.get_con_from_hdl(hdl);

  std::string body = con->get_request_body();
  std::string uri = con->get_resource();
  std::string met = con->get_request().get_method();
  WebInterfaceCallbackClient::HttpResponse response;
  for(CallbackClientsMap::iterator i = callback_clients_.begin();
      i!= callback_clients_.end();i++)
  {
        if(i->second->WebInterfaceHttpMessage(met,uri,body, &response)){
          break;
        }
  }

  con->set_body(response.response_message);
  con->set_status((websocketpp::http::status_code::value)response.response_code);
}

void WebInterface::OnMessage(connection_hdl hdl, WebSocketServer::message_ptr msg){
  LOG(DEBUG) << "WebInterface onMessage";
  std::string response;
  for(CallbackClientsMap::iterator i = callback_clients_.begin();
      i!= callback_clients_.end(); i++)
  {
    if(i->second->WebInterfaceWebSocketMessage(msg->get_payload(),&response)){
      break;
    };
  }
  if (response.length() > 0){
    SendMessage(response);
  }

}


bool WebInterface::Start(){
  LOG(DEBUG) << "Webinterface start on port " << port_;
  bool rv = false;
  try{
    server_.set_access_channels(websocketpp::log::alevel::all);
    server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    server_.init_asio();
    server_.listen(port_);
    server_.set_open_handler(bind(&WebInterface::OnOpen,this, ::_1));
    server_.set_close_handler(bind(&WebInterface::OnClose,this,::_1));
    server_.set_http_handler(bind(&WebInterface::OnHttp,this,::_1));
    server_.set_message_handler(bind(&WebInterface::OnMessage,this,::_1,::_2));
    server_.start_accept();
    thread t([this]{
      this->server_.run();
    });
    rv = true;
    server_thread_ = move(t);
  }catch(websocketpp::exception const & e){
    LOG(ERROR)<<e.what();
  }
  return rv;
  
}

void WebInterface::Stop(){
  LOG(DEBUG) << "Webinterface stop ";
  server_.stop();
  if(server_thread_.joinable()){
    server_thread_.join();
  }
  
}

void WebInterface::SendMessage(std::string message){
  lock_guard<mutex> guard(connection_mutex_);
  LOG(DEBUG) << "WebInterface send message: " << message;
  
  for(auto con: connections_){
    server_.send(con,message,websocketpp::frame::opcode::value::text);
  }
}
