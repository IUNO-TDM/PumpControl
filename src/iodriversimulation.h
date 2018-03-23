#ifndef SRC_IODRIVERSIMULATION_H_
#define SRC_IODRIVERSIMULATION_H_

#include "iodriverinterface.h"
#include <thread>
#include <condition_variable>

class IoDriverSimulation: public IoDriverInterface {
    public:
        IoDriverSimulation();
        virtual ~IoDriverSimulation();

        virtual bool Init(const char* config_text);
        virtual void DeInit();
        virtual void RegisterCallbackClient(IoDriverCallback* client);
        virtual void UnregisterCallbackClient(IoDriverCallback* client);
        virtual void GetDesc(std::vector<IoDescription>& desc) const;
        virtual bool GetValue(const std::string& name) const;
        virtual void SetValue(const std::string& name, bool value);

    private:
        void Simulate();
        IoDriverCallback* client_;
        std::thread* sim_thread_;
        std::condition_variable exit_condition_;
        std::mutex exit_mutex_;
        bool cabinet_door_switch_;
        bool juice_door_switch_;
        bool start_button_;
        bool start_button_illumination_;
};

#endif /* SRC_IODRIVERSIMULATION_H_ */
