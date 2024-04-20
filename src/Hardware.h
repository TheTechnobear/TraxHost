#pragma once


#include "Module.h"

class TraxHardwareImpl;

namespace TraxHost {
    class Hardware {
    public:
        Hardware();
        ~Hardware();

        bool init();
        bool handleEvents(Module&m);

    private:
        TraxHardwareImpl *impl_= nullptr;
    };
}