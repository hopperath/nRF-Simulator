#ifndef ETHER_H
#define ETHER_H

#include <Poco/Timer.h>
#include <Poco/Mutex.h>

#include "msgframe.h"

class Ether
{
public:
    explicit Ether();
//signals:
    void dispachMsg(tMsgFrame * theMSG);
    void collisionSig();

//slots
public:
    void sendMSG(tMsgFrame * theMSG);

private:
    Poco::Mutex mutex;
    std::unique_ptr<Poco::Timer> myTimer;
    tMsgFrame* MSG;

    void startTimer(int time);

//slots
private:
    void alarm(Poco::Timer& timer);

};

#endif // ETHER_H
