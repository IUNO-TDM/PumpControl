#include <webinterface.h>
#include <websocketpp/common/thread.hpp>
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
  mPort = port;
  // callbackclients = new CallbackClientsMap();
  mServer.set_access_channels(websocketpp::log::alevel::all);
  mServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
  mServer.init_asio();
  mServer.listen(mPort);
  mServer.set_open_handler(bind(&WebInterface::on_open,this, ::_1));
  mServer.set_close_handler(bind(&WebInterface::on_close,this,::_1));
  mServer.set_http_handler(bind(&WebInterface::on_http,this,::_1));
  mServer.set_message_handler(bind(&WebInterface::on_message,this,::_1,::_2));

}

void WebInterface::registerCallbackClient(WebInterfaceCallbackClient *client){
  callbackClients[client->getClientName()] = client;
}
void WebInterface::unregisterCallbackClient(WebInterfaceCallbackClient *client){
  callbackClients.erase(client->getClientName());
}

void WebInterface::on_open(connection_hdl hdl){
  printf("Juhuu open\n" );
}

void WebInterface::on_close(connection_hdl hdl){
  printf("close\n" );
}

void WebInterface::on_http(connection_hdl hdl){
  printf("http\n" );
  server::connection_ptr con = mServer.get_con_from_hdl(hdl);

  std::string body = con->get_request_body();
  std::string uri = con->get_resource();
  std::string met = con->get_request().get_method();
  WebInterfaceCallbackClient::http_response_struct response;
  for(CallbackClientsMap::iterator i = callbackClients.begin();
      i!= callbackClients.end();i++)
  {
        if(i->second->httpMessage(met,uri,body, &response)){
          break;
        }
  }

  con->set_body(response.responseMessage);
  con->set_status((websocketpp::http::status_code::value)response.responseCode);
}

void WebInterface::on_message(connection_hdl hdl, server::message_ptr msg){
  string response;
  for(CallbackClientsMap::iterator i = callbackClients.begin();
      i!= callbackClients.end(); i++)
  {
    if(i->second->webSocketMessage(msg->get_payload(),&response)){
      break;
    };
  }
  if (response.length() > 0){
    sendMessage(response);
  }

  printf("message\n" );
}


void WebInterface::start(){
  mServer.start_accept();
  thread t([this]{
    this->mServer.run();
  });
  mServerThread = move(t);
}

void WebInterface::stop(){
  mServer.stop();
  mServerThread.join();
}

void WebInterface::sendMessage(std::string message){
  printf("should send a message\n" );
}
