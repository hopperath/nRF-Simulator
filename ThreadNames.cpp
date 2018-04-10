//
// Created by Hopper II, Alan T on 4/9/18.
//

#include "ThreadNames.h"

#include <thread>

using namespace std;


int ThreadNames::counter = 1;
mutex ThreadNames::m;
map<std::thread::id,string> ThreadNames::tnames;

const string& ThreadNames::getName()
{
    auto id =this_thread::get_id();
    string& tid = tnames[id];
    if (tid.empty())
    {
        tid = to_string(counter++);
    }

    return tid;
}

const string& ThreadNames::getName(thread::id id)
{
    string& tid = tnames[id];

    if (tid.empty())
    {
        tid = to_string(counter++);
    }

    return tid;
}

void ThreadNames::setName(const char* name)
{
    lock_guard<mutex> lock(m);
    tnames.insert(pair<thread::id, string>(this_thread::get_id(),string(name)));
}

void ThreadNames::setName(const string& name)
{
    lock_guard<mutex> lock(m);
    tnames.insert(pair<thread::id, string>(this_thread::get_id(),name));
}

void ThreadNames::setName(thread::id tid, const std::string& name)
{
    lock_guard<mutex> lock(m);
    tnames.insert(pair<thread::id, string>(tid,name));
}

int ThreadNames::getCPU()
{
        int CPU;
        uint32_t CPUInfo[4];
        CPUID(CPUInfo, 1, 0);
        /* CPUInfo[1] is EBX, bits 24-31 are APIC ID */
        if ( (CPUInfo[3] & (1 << 9)) == 0)
        {
          CPU = -1;  /* no APIC on chip */
        }
        else
        {
          CPU = (unsigned)CPUInfo[1] >> 24;
        }
        if (CPU < 0) CPU = 0;

    return CPU;
}