#include "Hardware.h"

#include "Log.h"

// we put this outside namespace since we will need to include
// various platform specific headers

#ifdef __APPLE__
// a null implementation
class TraxHardwareImpl {
public:
    TraxHardwareImpl() = default;
    virtual ~TraxHardwareImpl() = default;
    bool init() { return true; }
    void handleEvents(TraxHost::Module &m) {}
};
#else

#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <unistd.h>


class TraxHardwareImpl {
public:
    TraxHardwareImpl() = default;
    virtual ~TraxHardwareImpl();
    bool init();
    void handleEvents(TraxHost::Module &m);

private:
    static constexpr unsigned NUM_ENCODERS = 4;
    int encFD_[NUM_ENCODERS] = { -1, -1, -1, -1 };
    int encSwitchFD_ = -1;
    int buttonFD_ = -1;

    const int encMap_[NUM_ENCODERS] = { 1, 3, 0, 2 };
    const int encSwMap_[NUM_ENCODERS] = { 1, 3, 0, 2 };
    int btnMap(int code) {
        // TraxHost::log("button code: " + std::to_string(code));
#ifdef TARGET_SSP
        switch (code) {
            case 88: return TraxHost::Module::SSPButtons::SSP_Soft_1;
            case 87: return TraxHost::Module::SSPButtons::SSP_Soft_2;
            case 68: return TraxHost::Module::SSPButtons::SSP_Soft_3;
            case 67: return TraxHost::Module::SSPButtons::SSP_Soft_4;
            case 64: return TraxHost::Module::SSPButtons::SSP_Soft_5;
            case 63: return TraxHost::Module::SSPButtons::SSP_Soft_6;
            case 62: return TraxHost::Module::SSPButtons::SSP_Soft_7;
            case 61: return TraxHost::Module::SSPButtons::SSP_Soft_8;
            case 65: return TraxHost::Module::SSPButtons::SSP_Up;
            case 59: return TraxHost::Module::SSPButtons::SSP_Down;
        }
#else
        switch (code) {
            case 59: return TraxHost::Module::SSPButtons::SSP_Soft_1;
            case 60: return TraxHost::Module::SSPButtons::SSP_Soft_2;
            case 61: return TraxHost::Module::SSPButtons::SSP_Soft_3;
            case 62: return TraxHost::Module::SSPButtons::SSP_Soft_4;
            case 64: return TraxHost::Module::SSPButtons::SSP_Soft_5;
            case 65: return TraxHost::Module::SSPButtons::SSP_Soft_6;
            case 66: return TraxHost::Module::SSPButtons::SSP_Soft_7;
            case 67: return TraxHost::Module::SSPButtons::SSP_Soft_8;
            case 68: return TraxHost::Module::SSPButtons::SSP_Up;
            case 63: return TraxHost::Module::SSPButtons::SSP_Down;
        }
#endif
        return -1;
    }
};

TraxHardwareImpl::~TraxHardwareImpl() {
    for (int i = 0; i < NUM_ENCODERS; i++) {
        if (encFD_[i] >= 0) close(encFD_[i]);
    }
    if (encSwitchFD_ >= 0) close(encSwitchFD_);
    if (buttonFD_ >= 0) close(buttonFD_);
}

bool TraxHardwareImpl::init() {
    //  open the encoders
    for (int i = 0; i < NUM_ENCODERS; i++) {
        static constexpr unsigned MAX_PATH = 100;
        char path[MAX_PATH];
        snprintf(path, MAX_PATH, "/dev/input/by-path/platform-rotary@%d-event", i);
        encFD_[i] = open(path, O_RDONLY | O_NONBLOCK);
        if (encFD_[i] < 0) {
            std::string msg = "hardware: failed to open encoder : " + std::to_string(i);
            TraxHost::error(msg);
            return false;
        }
    }

    // open the enc switch
    encSwitchFD_ = open("/dev/input/by-path/platform-gpio-keys-event", O_RDONLY | O_NONBLOCK);
    if (encSwitchFD_ < 0) {
        TraxHost::error("hardware: failed to open encoder switch");
        return false;
    }

    // open the buttons
    buttonFD_ = open("/dev/input/by-path/platform-matrix-keypad-event", O_RDONLY | O_NONBLOCK);
    if (buttonFD_ < 0) {
        TraxHost::error("hardware: failed to open button");
        return false;
    }

    return true;
}

#ifdef TARGET_SSP
static constexpr int encMult = 1;
#else
static constexpr int encMult = -1;
#endif

void TraxHardwareImpl::handleEvents(TraxHost::Module &m) {
    bool moreEvents = true;
    while (moreEvents) {
        struct input_event ev;
        moreEvents = false;
        for (int i = 0; i < NUM_ENCODERS; i++) {
            if (read(encFD_[i], &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_REL && ev.code == REL_X) {
                    // std::string msg = "encoder event: " + std::to_string(i) + " " + std::to_string(ev.type) + " " +
                    //                   std::to_string(ev.code) + " " + std::to_string(ev.value);
                    // TraxHost::log("Hardware: " + msg);
                    int enc = encMap_[i];
                    int val = ev.value * encMult;
                    if (val != 0) m.encoderTurned(enc, val);
                }
                moreEvents = true;
            }
        }
        if (read(encSwitchFD_, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                // std::string msg = "encoder switch: " + std::to_string(ev.type) + " " + std::to_string(ev.code) + " "
                // +
                //                   std::to_string(ev.value);
                // TraxHost::log("Hardware: " + msg);
                int enc = encSwMap_[ev.code - 2];
                if (enc >= 0 && enc < NUM_ENCODERS) m.encoderPressed(enc, ev.value);
            }
            moreEvents = true;
        }

        if (read(buttonFD_, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                // std::string msg = "button event: " + std::to_string(ev.type) + " " + std::to_string(ev.code) + " " +
                //                   std::to_string(ev.value);
                // TraxHost::log("Hardware: " + msg);
                int btn = btnMap(ev.code);
                if (btn >= TraxHost::Module::SSP_Soft_1 && btn < TraxHost::Module::SSP_LastBtn)
                    m.buttonPressed(btn, ev.value);
            }
            moreEvents = true;
        }
    }
}


#endif


/////////////////////////////////////////////////
// simply forwards to the implementation
namespace TraxHost {
Hardware::Hardware() : impl_(nullptr) {
}
Hardware::~Hardware() {
    delete impl_;
}
bool Hardware::init() {
    if (!impl_) impl_ = new TraxHardwareImpl();
    return impl_->init();
}

bool Hardware::handleEvents(Module &m) {
    if (impl_) impl_->handleEvents(m);
    return true;
}
}  // namespace TraxHost
