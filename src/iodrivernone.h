#ifndef SRC_IODRIVERNONE_H_
#define SRC_IODRIVERNONE_H_

#include "iodriverinterface.h"

class IoDriverNone: public IoDriverInterface {
    public:
        IoDriverNone();
        virtual ~IoDriverNone();

        virtual bool Init(const char* config_text);
        virtual void DeInit();
        virtual void RegisterCallbackClient(IoDriverCallback* client);
        virtual void UnregisterCallbackClient(IoDriverCallback* client);
        virtual void GetDesc(std::vector<IoDescription>& desc) const;
        virtual bool GetValue(const std::string& name) const;
        virtual void SetValue(const std::string& name, bool value);

    private:
        IoDriverCallback* client_;
};

#endif /* SRC_IODRIVERNONE_H_ */
