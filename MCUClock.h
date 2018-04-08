//
// Created by Hopper II, Alan T on 4/7/18.
//

#ifndef NRF_SIMULATOR_MCUCLOCK_H
#define NRF_SIMULATOR_MCUCLOCK_H

#include <chrono>


class MCUClock
{
    std::chrono::steady_clock::time_point start;


    public:
       MCUClock();
       void restart();
       uint32_t millis();
};

#endif //NRF_SIMULATOR_MCUCLOCK_H
