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
            startTimer(2);
        }
    }
    else
    {
        printf("Ether::collision\n");
        //TODO: revisit this, currently blocks the next RX by this caller, does that model reality?
        //collisionSig();
    }
}

void Ether::propogate(Timer& timer)
{
    printf("Ether::propogate\n");

    leaveEther();
    //mMsg = nullptr;

    //Ether free
    locked = false;
}

void Ether::startTimer(int time)
{

    printf("Ether::startTimer %d\n",time);
    myTimer = std::unique_ptr<Timer>(new Timer(time, 0));
    myTimer->start(TimerCallback<Ether>(*this, &Ether::propogate));
}