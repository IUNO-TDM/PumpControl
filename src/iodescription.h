#ifndef SRC_IODESCRIPTION_H_
#define SRC_IODESCRIPTION_H_

#include <string>

struct IoDescription{
    enum IoType{INPUT,OUTPUT};
    std::string name_;
    IoType type_;
};

#endif /* SRC_IODESCRIPTION_H_ */
