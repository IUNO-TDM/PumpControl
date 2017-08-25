#include "pumpcontrol.h"

void PumpControl::DecryptPrivate(const CryptoBuffer& in, CryptoBuffer& out){
    out.set(in, in.size());
}
