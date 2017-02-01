#include <unistd.h>
#include <string>


class WebInterfaceCallbackClient{
public:
  typedef struct{
      int responseCode;
      std::string responseMessage;
    }http_response_struct;
  virtual bool httpMessage(std::string method, std::string path, std::string body, http_response_struct *response)=0;
  virtual bool webSocketMessage(std::string message, std::string * response)=0;
  std::string getClientName();
protected:
  void setClientName(std::string);


private:
  std::string callbackName;
};
