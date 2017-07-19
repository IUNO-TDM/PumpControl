#include <unistd.h>
#include <string>


class WebInterfaceCallbackClient{
public:
  struct HttpResponse{
      int response_code;
      std::string response_message;
    };
  virtual ~WebInterfaceCallbackClient(){};

  virtual bool WebInterfaceHttpMessage(std::string method, std::string path, std::string body, HttpResponse *response)=0;
  virtual bool WebInterfaceWebSocketMessage(std::string message, std::string * response)=0;
  virtual void WebInterfaceOnOpen()=0;
  std::string GetClientName();

protected:
  void SetClientName(std::string);

private:
  std::string callback_name_;
};
