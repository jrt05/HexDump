
#include "Time.h"
#include "Windows.h"
#include <Windows.h>
#include <SDL.h>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>

namespace Time {
    time_p time_start;
    time_p time_last;

    void start() {
        time_start = Time::now();
        return;
    }

    // Get nanoseconds since start time
    long long getNanoSeconds() {
        time_p t_now = Time::now();
        long long ret = std::chrono::duration_cast<std::chrono::nanoseconds>(t_now - time_start).count();
        return ret;
    }
    // Get delta time since last time
    float getDelta() {
        time_p t_now = Time::now();
        float delta = std::chrono::duration_cast<fsec>(t_now - time_last).count();
        time_last = t_now;

        return delta;
    }

    // Sleep for specified milliseconds
    void sleep(unsigned int m) {
        timeBeginPeriod(1);
        SDL_Delay(m);
        timeEndPeriod(1);

        return;
    }

    // Return a string of current time
    std::string getTimeString() {
        std::string ret;
        std::stringstream ss;

        time_t start_time = time(0);   // Get time now
        struct tm *curr_time = new(struct tm);
        localtime_s(curr_time, &start_time);

        // Build time string
        ret = std::to_string(curr_time->tm_year + 1900) + "/" + std::to_string(curr_time->tm_mon + 1) + "/" +
            std::to_string(curr_time->tm_mday) + "-" + std::to_string(curr_time->tm_hour) + ":" +
            std::to_string(curr_time->tm_min) + ":" + std::to_string(curr_time->tm_sec);
        /*
        ret = std::to_string(curr_time->tm_year + 1900) + "/";
        ss << std::setw(2) << std::setfill('0') << std::to_string(curr_time->tm_mon + 1);
        ret += ss.str() + "/";
        ss << std::setw(2) << std::setfill('0') << std::to_string(curr_time->tm_mday);
        ret += ss.str() + "-";
        ss << std::setw(2) << std::setfill('0') << std::to_string(curr_time->tm_hour);
        ret += ss.str() + ":";
        ss << std::setw(2) << std::setfill('0') << std::to_string(curr_time->tm_min);
        ret += ss.str() + ":";
        ss << std::setw(2) << std::setfill('0') << std::to_string(curr_time->tm_sec);
        ret += ss.str();
        */

        return ret;
    }
}