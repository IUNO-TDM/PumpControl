#ifndef PUMPDRIVERCALLBACKCLIENT_H_
#define PUMPDRIVERCALLBACKCLIENT_H_

class PumpDriverCallbackClient {
    public:
        virtual ~PumpDriverCallbackClient() {
        }
        virtual void PumpDriverAmountWarning(int pump_number, int amountWarningLimit) = 0;
};

#endif
