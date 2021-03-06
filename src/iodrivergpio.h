#ifndef SRC_IODRIVERGPIO_H_
#define SRC_IODRIVERGPIO_H_

#include "iodriverinterface.h"
#include <map>
#include <thread>
#include <condition_variable>

class IoDriverGpio: public IoDriverInterface {
    public:
        enum GpioType{
            OUTPUT,
            INPUT,
            INPUT_PULLUP,
            INPUT_PULLDOWN
        };
        enum GpioPolarity{
            ACTIVE_LOW,
            ACTIVE_HIGH
        };

        IoDriverGpio();
        virtual ~IoDriverGpio();

        virtual bool Init(const char* config_text);
        virtual void DeInit();
        virtual void RegisterCallbackClient(IoDriverCallback* client);
        virtual void UnregisterCallbackClient(IoDriverCallback* client);
        virtual void GetDesc(std::vector<IoDescription>& desc) const;
        virtual bool GetValue(const std::string& name) const;
        virtual void SetValue(const std::string& name, bool value);

    private:
        struct GpioDesc{
            size_t pin_;
            GpioType type_;
            GpioPolarity polarity_;
            bool current_value_;
        };

        void PollLoop();
        bool Read(const GpioDesc& gd, const std::string& name);
        void Write(GpioDesc& gd, bool value, const std::string& name);
        static GpioType ParsePinType(const std::string& str);
        static GpioPolarity ParsePolarity(const std::string& str);

        std::map<std::string, GpioDesc> gpios_;
        IoDriverCallback* client_;
        bool gpio_initialized_;
        std::thread* poll_thread_;
        std::condition_variable exit_condition_;
        std::mutex exit_mutex_;
};

#endif /* SRC_IODRIVERGPIO_H_ */
