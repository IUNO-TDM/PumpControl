#include <webinterfacecallbackclient.h>

void WebInterfaceCallbackClient::SetClientName(std::string name){
  callback_name_ = name;
}

std::string WebInterfaceCallbackClient::GetClientName(){
  return callback_name_;
}
