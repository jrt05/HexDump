#pragma once

#include <chrono>
#include <string>

namespace Time {
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::high_resolution_clock::time_point time_p;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::duration<float> fsec;

    const long long SECOND = 1000000000ll;      // One second is a billion nanoseconds

    float getDelta();
    long long getNanoSeconds();
    void start();
    void sleep(unsigned int milliseconds);
    std::string getTimeString();
}