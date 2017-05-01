#pragma once

#include <fstream>
#include <string>

namespace Logger {
    class Log {
    public:
        std::ofstream logstream;

        Log();
        Log(std::string);
        ~Log();
        bool isVerbose();
    private:
        bool verbose;
    };
    extern Log LogOut;
}