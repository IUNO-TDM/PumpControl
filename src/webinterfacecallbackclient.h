#ifndef WEBINTERFACECALLBACKCLIENT_H
#define WEBINTERFACECALLBACKCLIENT_H

#include <string>
#include <unistd.h>

class WebInterfaceCallbackClient {
    public:
        struct HttpResponse {
                int response_code;
                std::string response_message;
        };

        virtual ~WebInterfaceCallbackClient() {
        }

        virtual bool WebInterfaceHttpMessage(std::string method, std::string path, std::string body,
                HttpResponse *response)=0;
        virtual void WebInterfaceOnOpen()=0;
        std::string GetClientName();

    protected:
        void SetClientName(std::string);

    private:
        std::string callback_name_;
};

#endif
