#ifndef NRF24L01PLUS_H
#define NRF24L01PLUS_H

#include <stdint.h>
#include "nRF24interface.h"

class Ether;

#include <poco/Timer.h>
#include <queue>
#include <memory>

class nRF24l01plus : public nRF24interface
{
    public:
        nRF24l01plus(Ether* someEther = NULL);
        virtual ~nRF24l01plus();
    protected:
    private:
        void send_frame(tMsgFrame * theFrame);
        //Ether interface placeholders
        void send_to_ether(tMsgFrame * theFrame);
        void startPTX();
        tMsgFrame* TXpacket;
        std::unique_ptr<Poco::Timer> theTimer;
        uint64_t ACK_address;
        bool waitingForACK;
        bool collision;
        std::shared_ptr<Ether> theEther;
        void ackReceived(tMsgFrame * theMSG, byte pipe);

        void CEsetHIGH();
        void TXmodeSet();
        void TXpacketAdded();
        void PWRUPset(void);
        void noACKalarm(Poco::Timer& timer);
        void receiveMsgFromEther(tMsgFrame * theMSG);
        void setCollision();
        void startTimer(int time);


        void sendMsgToEther(tMsgFrame* theMSG);
};

#endif // NRF24L01PLUS_H
