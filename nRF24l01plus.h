#ifndef NRF24L01PLUS_H
#define NRF24L01PLUS_H

#include <cstdint>
#include "nRF24interface.h"

class Ether;

#include <Poco/BasicEvent.h>
#include <Poco/Timer.h>
#include <queue>
#include <memory>

class nRF24l01plus : public nRF24interface
{
    public:
        Poco::BasicEvent<tMsgFrame> sendMsgEvent;

        bool waitingForAck = false;
        bool sendingAck = false;
        tMsgFrame* ackFrame;
        bool collision = false;
        std::shared_ptr<Ether> theEther;
        tMsgFrame* TXpacket;
        uint64_t ACK_address;
        Poco::Timer theTimer;
        //Poco::TimerCallback<nRF24l01plus>* noACKalarmCallback;
        std::unique_ptr<Poco::TimerCallback<nRF24l01plus>> noACKalarmCallback;


    public:
        explicit nRF24l01plus(int id, Ether* someEther = nullptr);
        ~nRF24l01plus() override;
    protected:
    private:
        void startPTX();
        void ackReceived(tMsgFrame* theMSG, byte pipe);

        void CEsetHIGH() override ;
        void TXmodeSet() override ;
        void PWRUPset() override ;
        void TXpacketAdded();
        void noACKalarm(Poco::Timer& timer);
        void setCollision();
        void startTimer(int time);
        void sendAutoAck(tMsgFrame* theFrame, byte pipe);

        //Ether interface placeholders
        void sendMsgToEther(tMsgFrame* theMSG);
        void receiveMsgFromEther(const void* pSender, tMsgFrame& theMSG);
};

#endif // NRF24L01PLUS_H
