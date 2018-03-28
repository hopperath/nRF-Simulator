#ifndef ETHER_H
#define ETHER_H

#include <Poco/Timer.h>
#include <Poco/Mutex.h>
#include <Poco/BasicEvent.h>

#include "msgframe.h"

class Ether
{
    private:
        //This is thread based
        Poco::Mutex mutex;
        bool locked = false;
        std::unique_ptr<Poco::TimerCallback<Ether>> alarmCallback;
        std::unique_ptr<Poco::Timer> myTimer;
        tMsgFrame mMsg;

    public:
        Poco::BasicEvent<tMsgFrame> dispatchMsgEvent;
        Poco::BasicEvent<void> collisionEvent;

    public:
        explicit Ether();
        //Event methods
        void leaveEther();
        void collisionSig();
        void enterEther(const void* pSender, tMsgFrame& theMSG);

    private:
        void propogate(Poco::Timer& timer);
        void startTimer(int time);

};

#endif // ETHER_H
