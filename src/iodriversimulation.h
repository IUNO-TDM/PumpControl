#ifndef SRC_IODRIVERSIMULATION_H_
#define SRC_IODRIVERSIMULATION_H_

#include <iodriverinterface.h>

class IoDriverSimulation: public IoDriverInterface {
    public:
        IoDriverSimulation();
        virtual ~IoDriverSimulation();

        virtual bool Init(const char* config_text);
        virtual void DeInit();
        virtual void GetDesc(std::string& desc) const;
};

#endif /* SRC_IODRIVERSIMULATION_H_ */
