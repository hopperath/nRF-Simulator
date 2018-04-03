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
        std::shared_ptr<tMsgFrame> ackPktFrame;
        bool collision = false;
        std::shared_ptr<Ether> theEther;
        std::shared_ptr<tMsgFrame> TXpacket;
        uint64_t ACK_address;
        Poco::Timer theTimer;
        //Poco::TimerCallback<nRF24l01plus>* noACKalarmCallback;
        std::unique_ptr<Poco::TimerCallback<nRF24l01plus>> noACKalarmCallback;

        //Thread for RF24 processor
        void runRF24();
        //std::thread chip(nRF24interface::run);
        std::thread chip;
        std::condition_variable cmdAvailable;
        std::mutex m;

    protected:
        static const int IDLE = 0;
        static const int PTX = 1;
        static const int PRX = 2;

        int pendingCmd = 0;

        void cmdNotify(int cmd);
        void processCmd();

    public:
        explicit nRF24l01plus(int id, Ether* someEther = nullptr);

    protected:
        std::shared_ptr<tMsgFrame> rxMsg;

    private:
        void startPTX();
        void startPRX();
        void ackReceived(std::shared_ptr<tMsgFrame> theMSG, byte pipe);

        void CEsetHIGH() override ;
        void TXmodeSet() override ;
        void PWRUPset() override ;
        void TXpacketAdded();
        void noACKalarm(Poco::Timer& timer);
        void setCollision();
        void startTimer(int time);
        void sendAutoAck(std::shared_ptr<tMsgFrame> theFrame, byte pipe);

        //Ether interface placeholders
        void sendMsgToEther(std::shared_ptr<tMsgFrame> theMSG);
        void receiveMsgFromEther(const void* pSender, tMsgFrame& theMSG);
};

#endif // NRF24L01PLUS_H
