#include <unistd.h>
#include <map>
#include <webinterfacecallbackclient.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
typedef websocketpp::server<websocketpp::config::asio> server;

typedef std::map<std::string,WebInterfaceCallbackClient*> CallbackClientsMap;

class WebInterface {
public:
  WebInterface(int port);
  void registerCallbackClient(WebInterfaceCallbackClient *);
  void unregisterCallbackClient(WebInterfaceCallbackClient *);

  void start();
  void stop();
  void sendMessage(std::string message);
private:
  CallbackClientsMap callbackClients;
  int mPort;
  websocketpp::server<websocketpp::config::asio> mServer;
  std::thread mServerThread;
  void on_open(websocketpp::connection_hdl hdl);

  void on_close(websocketpp::connection_hdl hdl);

  void on_http(websocketpp::connection_hdl hdl);

  void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);
};
