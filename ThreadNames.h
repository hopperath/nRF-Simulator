//
// Created by Hopper II, Alan T on 4/9/18.
//

#ifndef NRF_SIMULATOR_THREADNAME_H
#define NRF_SIMULATOR_THREADNAME_H

#include <string>
#include <map>
#include <thread>

class ThreadNames
{
    protected:
        static std::mutex m;
        static std::map<std::thread::id,std::string> tnames;

    public:
        static int counter;
        static const std::string& getName(std::thread::id);
        static void setName(std::thread::id, const std::string& name);
        static void setName(const std::string& name);
        static void setName(const char* name);
        static const std::string& getName();
};

#endif //NRF_SIMULATOR_THREADNAME_H
