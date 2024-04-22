#include "Log.h"

#ifdef __APPLE__
#include <iostream>
#include <thread>
#else
#include <fstream>
#include <iostream>
#include <thread>
#endif

namespace TraxHost {

#ifdef __APPLE__
void log(const std::string &m) {
    std::cerr << std::this_thread::get_id() << " : " << m << std::endl;
}

#else
void log(const std::string &m) {
    std::ofstream s("/dev/kmsg");
    s << std::this_thread::get_id() << " : " << m << std::endl;
    std::cerr << std::this_thread::get_id() << " : " << m << std::endl;
}
#endif

void error(const std::string &m) {
    log("ERROR: " + m);
}

}  // namespace TraxHost
