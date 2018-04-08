//
// Created by Hopper II, Alan T on 4/7/18.
//

#include "MCUClock.h"


using namespace std;
using namespace chrono;

MCUClock::MCUClock()
{
    start = steady_clock::now();
}

void MCUClock::restart()
{
    start = steady_clock::now();
}

uint32_t MCUClock::millis()
{
    auto now = steady_clock::now();
    auto millis = duration_cast<milliseconds>(now - start);

    return (uint32_t) millis.count();
}

