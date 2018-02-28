#ifndef SRC_IODRIVERCALLBACK_H_
#define SRC_IODRIVERCALLBACK_H_

#include <string>

class IoDriverCallback {
    public:

        virtual ~IoDriverCallback() {
        }

        virtual void NewInputState(const char* name, bool value) = 0;
};

#endif /* SRC_IODRIVERCALLBACK_H_ */
