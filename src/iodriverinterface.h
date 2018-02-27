#ifndef SRC_IODRIVERINTERFACE_H_
#define SRC_IODRIVERINTERFACE_H_

#include <string>

class IoDriverInterface {
    public:

        virtual ~IoDriverInterface() {
        }

        virtual bool Init(const char* config_text) = 0;

        virtual void DeInit() = 0;

        virtual void GetDesc(std::string& desc) const = 0;
};

#endif /* SRC_IODRIVERINTERFACE_H_ */
