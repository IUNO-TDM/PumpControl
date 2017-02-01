#include <webinterfacecallbackclient.h>

void WebInterfaceCallbackClient::setClientName(std::string name){
  callbackName = name;
}

std::string WebInterfaceCallbackClient::getClientName(){
  return callbackName;
}
