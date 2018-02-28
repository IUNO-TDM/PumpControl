#ifndef SRC_IODRIVERINTERFACE_H_
#define SRC_IODRIVERINTERFACE_H_

#include "iodrivercallback.h"
#include "iodescription.h"
#include <vector>

class IoDriverInterface {
    public:
        virtual ~IoDriverInterface() {}
        virtual bool Init(const char* config_text) = 0;
        virtual void DeInit() = 0;
        virtual void RegisterCallbackClient(IoDriverCallback* client) = 0;
        virtual void UnregisterCallbackClient(IoDriverCallback* client) = 0;
        virtual void GetDesc(std::vector<IoDescription>& desc) const = 0;
        virtual bool GetValue(const std::string& name) const = 0;
        virtual void SetValue(const std::string& name, bool value) = 0;
};

#endif /* SRC_IODRIVERINTERFACE_H_ */
