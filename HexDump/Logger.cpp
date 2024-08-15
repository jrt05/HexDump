
#include "Logger.h"
#include "Time.h"
#include <fstream>
#include <atlbase.h>

namespace Logger {

    Log LogOut;

    Log::Log() : verbose(false) {
        logstream.open("./messages.log", std::ofstream::out | std::ofstream::app);
        logstream << "------------------------------------------------------------------" << std::endl;
        logstream << Time::getTimeString() << " Program started" << std::endl;
        return;
    }
    Log::Log(std::string file) : verbose(false) {
        logstream.open(file.c_str(), std::ofstream::out | std::ofstream::app);
        logstream << "------------------------------------------------------------------" << std::endl;
        logstream << Time::getTimeString() << " Program started" << std::endl;
        return;
    }
    Log::~Log() {
        logstream << Time::getTimeString() << " Program ended" << std::endl;
        logstream.close();

        return;
    }
    bool Log::isVerbose() {
        return verbose;
    }

    //std::ofstream logstream;
}