#include <Poco/Delegate.h>
#include "nRF24l01plus.h"
#include "ether.h"

using namespace std;
using namespace Poco;

//constructor
nRF24l01plus::nRF24l01plus(string& id, Ether* someEther) : theEther(someEther)
{
    this->id = id;
    //ctor

    //inbound events
    theEther->dispatchMsgEvent += Poco::delegate(this, &nRF24l01plus::receiveMsgFromEther);
    theEther->collisionEvent += Poco::delegate(this, &nRF24l01plus::setCollision);

    //outbound events
    sendMsgEvent += Poco::delegate(someEther, &Ether::enterEther);

    noACKalarmCallback = unique_ptr<TimerCallback<nRF24l01plus>>(new TimerCallback<nRF24l01plus>(*this, &nRF24l01plus::noACKalarm));
    /*
    connect(this,SIGNAL(CEsetHIGH()),this,SLOT(CEsetHIGH()));
    connect(this,SIGNAL(TXmodeSet()),this,SLOT(TXmodeSet()));
    connect(this,SIGNAL(PWRUPset()),this,SLOT(PWRUPset()));
    connect(this,SIGNAL(TXpacketAdded()),this,SLOT(TXpacketAdded()));
    */

    /*
    connect(theEther, SIGNAL(dispachMsg(tMsgFrame * )), this, SLOT(reciveMsgFromET(tMsgFrame * )));
    connect(this, SIGNAL(sendMsgToET(tMsgFrame * )), theEther, SLOT(sendMSG(tMsgFrame * )));
    connect(theEther, SIGNAL(coalisionSig()), this, SLOT(setCoalision()));
    */
}

void nRF24l01plus::receiveMsgFromEther(const void* pSender, tMsgFrame& theMSG)
{
    printf("%s: nRF24l01plus::receiveMsgFromEther collision=%d\n", id.c_str(), collision);
    if (!collision)
    {//coalision did not happen
        printf("%s: pwrup=%d  ce=%d rx_mode=%d waitingForAck=%d\n", id.c_str(), isPWRUP(), getCE(), isRX_MODE(), waitingForACK);
        printf("%s: msg: %s\n", id.c_str(), theMSG.toString().c_str());
        if (isPWRUP() && getCE())
        {//in Standby mode (PWRUP = 1 && CE = 1)
            byte pipe = addressToPipe(theMSG.Address);
            //printf("%s: pipe=%u\n", id.c_str(), pipe);
            if (isRX_MODE())
            {//receiving
                //check if address is one off th
                if (pipe!=0xFF)
                {//pipe is open ready to receve
                    //fill RX buffer1
                    receive_frame(&theMSG, pipe);
                }
            }
            else if (waitingForACK)
            {//waiting for ack
                if (pipe==addressToPipe(getTXaddress()))
                {//addess is the P0 address, this is ACK packet
                    ackReceived(&theMSG, pipe);
                }
            }
        }
        else
        {

        }
    }
    collision = false;
}

nRF24l01plus::~nRF24l01plus()
{
    //dtor
}

void nRF24l01plus::sendMsgToEther(tMsgFrame* theMSG)
{
    printf("%s: sendMsgToEther:msg.addr=%llx\n",id.c_str(),theMSG->Address);
    sendMsgEvent.notifyAsync(this, *theMSG);
}

/*
 * Start transmision...
 *
 */
void nRF24l01plus::startPTX()
{
    printf("%s: nRF24l01plus::startPTX\n", id.c_str());
    if (getCE()==false || isPWRUP()==false)
    {
        return;
    }

    if (isRX_MODE())
    {
        return;
    }

    tMsgFrame* packetToSend = getTXpacket();
    if (packetToSend==nullptr)
    {
        return;
    }
    //packet found in TX that is not ACK packet
    //load with TX address
    packetToSend->Address = getTXaddress();
    sendMsgToEther(packetToSend);
    //check if ack expected
    if ((packetToSend->Packet_Control_Field.NP_ACK==0) && (getARC()!=0))
    {
        //set for ACK recipt..
        ACK_address = getTXaddress();
        waitingForACK = true;
        clearARC_CNT();
        printf("%s: Autoretry delay ARD=%u\n", id.c_str(), getARD());
        startTimer(getARD() * 10);
        TXpacket = packetToSend;
    }
    else
    {//no ack last transmited is the one that is
        if (lastTransmited!=nullptr)
        {
            delete lastTransmited;
            lastTransmited = nullptr;
        }
        lastTransmited = packetToSend;
        setTX_DS_IRQ();
    }
}

/*
 * in PTX if CE is set HIGH and there is a packet waiting transmit packet
 *
 */
void nRF24l01plus::CEsetHIGH()
{
    //if nRF is in RX mode nothing shoud happen;
    if (isRX_MODE()==true || isPWRUP()==false)
    { return; }
    startPTX();
}

/*
 * If TX mode is set while CE is high and there is a packet waiting transmit packet
 *
 */
void nRF24l01plus::TXmodeSet()
{
    if (getCE()==false || isPWRUP()==false)
    { return; }
    startPTX();

}

void nRF24l01plus::TXpacketAdded()
{
    if (getCE()==false || isRX_MODE()==true || isPWRUP()==false)
    { return; }
    startPTX();
}

void nRF24l01plus::PWRUPset()
{
    if (getCE()==false || isRX_MODE()==true)
    { return; }
    startPTX();
}

void nRF24l01plus::ackReceived(tMsgFrame* theMSG, byte pipe)
{

    printf("%s: nRF24l01+::ackReceived\n", id.c_str());
    if (isDynamicACKEnabled())
    {//coud be ACK with payload
        receive_frame(theMSG, pipe);
        waitingForACK = false;
    }
    else
    {//could be regular ack
        if (theMSG->Packet_Control_Field.Payload_length==0)
        {//regular ACK receved :D
            waitingForACK = false;
        }
    }

    if (waitingForACK==false)
    {
        theTimer.stop();

        if (lastTransmited!=NULL)
        {
            delete lastTransmited;
            lastTransmited = NULL;
        }
        lastTransmited = TXpacket;
        setTX_DS_IRQ();
    }
}

void nRF24l01plus::noACKalarm(Timer& timer)
{
    printf("%s: nRF24l01+::noACKalarm\n", id.c_str());
    if (getARC_CNT()==getARC())
    {//number of max retransmits acchived
        //failed to reach targe set aproprite flags
        PLOS_CNT_INC();
        setMAX_RT_IRQ();
        if (lastTransmited!=NULL)
        {
            delete lastTransmited;
        }
        lastTransmited = TXpacket;
        waitingForACK = false;
    }
    else
    {//retransmit message and increasse reTransCounter and start timer again
        sendMsgToEther(TXpacket);
        //setup wait for ack...
        ARC_CNT_INC();
        startTimer(getARD() * 10);//10ms real life 250us
    }
}

void nRF24l01plus::setCollision()
{
    collision = true;
}

void nRF24l01plus::startTimer(int time)
{
    if (time > 0)
    {
        printf("%s: nRF24l01+::startTimer %d\n", id.c_str(), time);
        theTimer.setStartInterval(time);
        theTimer.setPeriodicInterval(0);
        theTimer.start(*noACKalarmCallback);
    }
}


