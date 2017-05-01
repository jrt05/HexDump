
#include "Logger.h"
#include "Time.h"
#include <fstream>
#include <atlbase.h>

namespace Logger {

    Log LogOut;

    Log::Log() {
        logstream.open("./messages.log", std::ofstream::out | std::ofstream::app);
        logstream << "------------------------------------------------------------------" << std::endl;
        logstream << Time::getTimeString() << " Program started" << std::endl;
        verbose = false;
        return;
    }
    Log::Log(std::string file) {
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