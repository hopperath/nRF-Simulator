#include "ether.h"
#include <stdio.h>

using namespace Poco;
using namespace std;

Ether::Ether()
{
    alarmCallback = unique_ptr<TimerCallback<Ether>>(new TimerCallback<Ether>(*this, &Ether::propogate));
}

void Ether::leaveEther()
{
    printf("Ether::leaveEther msg.addr=%llx\n",mMsg.Address);
    //No need for async, in timer thread
    //dispatchMsgEvent.notify(this, mMsg);
    dispatchMsgEvent.notify(this, mMsg);
}

void Ether::collisionSig()
{
    collisionEvent.notifyAsync(this);
}

void Ether::enterEther(const void* pSender, tMsgFrame& msg, bool isAck)
{
    printf("Ether::enterEther msg.addr=%llx\n",msg.Address);

    if (locked == false && mutex.try_lock())
    {
        locked = true;
        mutex.unlock();

        mMsg = msg;
        if (isAck)
        {
            startTimer(1);
        }
        else
        {
            startTimer(1);
        }
    }
    else
    {
        printf("Ether::collision msg dropped\n");
    }
}

void Ether::propogate(Timer& timer)
{
    printf("Ether::propogate\n");

    leaveEther();
    //mMsg = nullptr;

    //Ether free
    locked = false;
    printf("Ether::propogate complete\n");
}

void Ether::startTimer(int time)
{

    printf("Ether::startTimer %d\n",time);
    myTimer = std::unique_ptr<Timer>(new Timer(time, 0));
    myTimer->start(TimerCallback<Ether>(*this, &Ether::propogate),Thread::PRIO_HIGHEST);
}