#ifndef SRC_IODRIVERGPIO_H_
#define SRC_IODRIVERGPIO_H_

#include "iodriverinterface.h"
#include <map>

class IoDriverGpio: public IoDriverInterface {
    public:
        enum GpioType{
            OUTPUT,
            INPUT,
            INPUT_PULLUP,
            INPUT_PULLDOWN
        };
        IoDriverGpio();
        virtual ~IoDriverGpio();

        virtual bool Init(const char* config_text);
        virtual void DeInit();
        virtual void GetDesc(std::string& desc) const;

    private:
        struct GpioDesc{
            size_t pin_;
            GpioType type_;
        };
        std::map<std::string, GpioDesc> gpios_;
};

#endif /* SRC_IODRIVERGPIO_H_ */
