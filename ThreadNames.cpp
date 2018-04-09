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