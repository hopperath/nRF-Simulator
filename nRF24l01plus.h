#ifndef NRF24L01PLUS_H
#define NRF24L01PLUS_H



#include <Poco/BasicEvent.h>
#include <Poco/Timer.h>
#include <queue>
#include <memory>
#include <cstdint>
#include <condition_variable>
#include "nRF24interface.h"
#include "MCUClock.h"
#include <condition_variable>
#include <mutex>


class Ether;

class nRF24l01plus : public nRF24interface
{
    static const int ACK_TIMEOUT_FACTOR    = 7;

    protected:
        //States
        static const int S_PWRDOWN    = 0;
        static const int S_STANDBY_I  = 1;
        static const int S_STANDBY_II = 2;
        static const int S_RX_MODE    = 3;
        static const int S_TX_MODE    = 4;
        int radioState = S_PWRDOWN;
        void standbyTransition();
        void setRadioState(int state);
        const char* stateToString(int state);
        const char* stateToString();
        void setRX_MODE();
        void setTX_MODE();

        std::mutex txMutex;
        std::condition_variable txdone;
    public:
        void waitForTX(int timeout);
        void notifyTX();

    public:
        Poco::BasicEvent<tMsgFrame> sendMsgEvent;

        bool waitingForAck = false;
        std::shared_ptr<tMsgFrame> ackPktFrame;
        std::shared_ptr<Ether> theEther;
        std::shared_ptr<tMsgFrame> TXpacket;
        uint64_t ACK_address;
        //Poco::Timer theTimer;
        std::unique_ptr<Poco::Timer> theTimer;
        std::unique_ptr<Poco::TimerCallback<nRF24l01plus>> noACKalarmCallback;
        std::string logHdr() override;
        uint32_t millis();
        MCUClock mcuClock;

    protected:
        //Thread for RF24 processor
        void runRF24();
        std::thread chip;
        std::condition_variable cmdAvailable;
        std::mutex cmdPickup;
        std::mutex shockburst;

    protected:
        static const int IDLE   = 0;
        static const int PTX    = 1;
        static const int PRX    = 2;
        static const int PNOACK = 3;

        int pendingCmd = IDLE;

        bool running=true;
        void cmdNotify(int cmd);
        void processCmd();
        const char* cmdToString(int cmd);
        void processNoAck();


    public:
        explicit nRF24l01plus(int id, Ether* someEther, MCUClock& clock);
        void stop();

    protected:
        std::shared_ptr<tMsgFrame> rxMsg;

    private:
        void startPTX();
        void startPRX();
        void ackReceived(std::shared_ptr<tMsgFrame> theMSG, byte pipe);
        void clearAck();

        void CEset() override ;
        void RXTXmodeSet() override ;
        void PWRset() override ;
        void TXPacketAdded() override;
        void removeTXPacket(std::shared_ptr<tMsgFrame> frame) override;
        void noACKalarm(Poco::Timer& timer);
        void startTimer(int time);
        void sendAutoAck(std::shared_ptr<tMsgFrame> theFrame, byte pipe);

        //Ether interface placeholders
        void sendMsgToEther(std::shared_ptr<tMsgFrame> theMSG, bool isAck);
        void receiveMsgFromEther(const void* pSender, tMsgFrame& theMSG);
};

#endif // NRF24L01PLUS_H
