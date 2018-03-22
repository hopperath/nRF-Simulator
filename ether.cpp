#include "ether.h"
#include <stdio.h>

using namespace Poco;
using namespace std;

Ether::Ether()
{
}

void Ether::sendMSG(tMsgFrame *theMSG)
{
    printf("sendMSG\n");
    if(mutex.tryLock(0))
    {
        MSG = theMSG;
        startTimer(60);
    }
    else
    {
        //emit coalisionSig();
        collisionSig();
    }
}

void Ether::alarm(Timer& timer)
{
    printf("Ether::alarm\n");
    mutex.unlock();
    myTimer->stop();
    dispachMsg(MSG);
    MSG = nullptr;
}

void Ether::startTimer(int time)
{
    myTimer = std::unique_ptr<Timer>(new Timer(time, 0));
    myTimer->start(TimerCallback<Ether>(*this, &Ether::alarm));
}