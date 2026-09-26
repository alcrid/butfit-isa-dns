#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>

class Logger {
private:
    static bool verbose;

public:
    static void set_verbose(bool v) {
        verbose = v;
    }
    
    static void log(const std::string& message) {
        if (verbose) {
            std::cerr << message << std::endl;
        }
    }
};

#endif
