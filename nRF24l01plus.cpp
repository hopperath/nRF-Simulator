#include <Poco/Delegate.h>
#include "nRF24l01plus.h"
#include "ether.h"

using namespace std;
using namespace Poco;

//constructor
nRF24l01plus::nRF24l01plus(int id, Ether* someEther) : theEther(someEther)
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
    if (theMSG.radioId == 0)
    {
        printf("ERROR: radioId=0\n");
    }
    //Sent from this radio, ignore
    if (theMSG.radioId==id)
    {
        return;
    }

    printf("%d: nRF24l01plus::receiveMsgFromEther msg=%s collision=%d\n", id, theMSG.toString().c_str(),collision);
    if (!collision)
    {//collision did not happen
        printf("%d: pwrup=%d  ce=%d rx_mode=%d waitingForAck=%d\n", id, isPWRUP(), getCE(), isRX_MODE(), waitingForAck);

        if (waitingForAck)
        {//waiting for ack
            byte pipe = addressToPipe(theMSG.Address);
            printf("%d: addr=0x%llx  ack_addr=0x%llx", id, theMSG.Address,ACK_address);
            if (theMSG.Address == ACK_address)
            {//addess is the P0 address, this is ACK packet
                ackReceived(&theMSG, pipe);
            }
        }
        else if (isPWRUP() && getCE())
        {//in a Standby mode (PWRUP = 1 && CE = 1)
            byte pipe = addressToPipe(theMSG.Address);
            //printf("%d: pipe=%u\n", id, pipe);
            if (isRX_MODE())
            {//receiving
                //check if address is one off the pipe addresses
                if (pipe!=0xFF)
                {//pipe is open ready to receve
                    //fill RX buffer1
                    receive_frame(&theMSG, pipe);
                    sendAutoAck(&theMSG,pipe);
                }
            }

        }
    }
    collision = false;
}

void nRF24l01plus::sendAutoAck(tMsgFrame* theFrame, byte pipe)
{
    printf("%d: nRF24l01plus::sendAutoAck\n", id);
    if (theFrame->Packet_Control_Field.NO_ACK)
    {
        return;
    }
    //TODO:Check pipe AUTOACK

    //auto msg = get_ack_packet_for_pipe(pipe);

    ackFrame = new tMsgFrame;

    ackFrame->radioId = id;
    ackFrame->Packet_Control_Field.Payload_length = 0;
    ackFrame->Packet_Control_Field.NO_ACK =  0;
    ackFrame->Packet_Control_Field.PID = 0;
    ackFrame->Address = theFrame->Address;
    setTX_MODE();
    startPTX();
    setRX_MODE();
}

nRF24l01plus::~nRF24l01plus()
{
    //dtor
}

void nRF24l01plus::sendMsgToEther(tMsgFrame* theMSG)
{
    printf("%d: sendMsgToEther:msg.addr=%llx\n",id,theMSG->Address);
    sendMsgEvent.notifyAsync(this, *theMSG);
}

/*
 * Start transmision...
 *
 */
void nRF24l01plus::startPTX()
{
    printf("%d: nRF24l01plus::startPTX\n", id);
    if (getCE()==false || isPWRUP()==false)
    {
        return;
    }

    if (isRX_MODE())
    {
        return;
    }

    //If ack available, send
    //TODO: handle ack payload, does it retry?
    if (ackFrame)
    {
        sendMsgToEther(ackFrame);
        delete ackFrame;
        ackFrame = nullptr;
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
    if ((packetToSend->Packet_Control_Field.NO_ACK==0) && (getARC()!=0))
    {
        //set for ACK recipt..
        ACK_address = getTXaddress();
        waitingForAck = true;
        clearARC_CNT();
        printf("%d: Autoretry delay ARD=%u\n", id, getARD());
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
    {
        return;
    }
    startPTX();
}

/*
 * If TX mode is set while CE is high and there is a packet waiting transmit packet
 *
 */
void nRF24l01plus::TXmodeSet()
{
    if (getCE()==false || isPWRUP()==false)
    {
        return;
    }
    startPTX();

}

void nRF24l01plus::TXpacketAdded()
{
    if (getCE()==false || isRX_MODE()==true || isPWRUP()==false)
    {
        return;
    }
    startPTX();
}

void nRF24l01plus::PWRUPset()
{
    if (getCE()==false || isRX_MODE()==true)
    {
        return;
    }
    startPTX();
}

void nRF24l01plus::ackReceived(tMsgFrame* theMSG, byte pipe)
{

    printf("%d: nRF24l01+::ackReceived\n", id);
    if (isDynamicACKEnabled())
    {//coud be ACK with payload
        receive_frame(theMSG, pipe);
        waitingForAck = false;
    }
    else
    {//could be regular ack
        if (theMSG->Packet_Control_Field.Payload_length==0)
        {//regular ACK receved :D
            waitingForAck = false;
        }
    }

    if (waitingForAck==false)
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
    printf("%d: nRF24l01+::noACKalarm ARC_CNT=%u ARC=%u\n", id,getARC_CNT(),getARC());
    if (getARC_CNT()==getARC())
    {//number of max retransmits acchived
        //failed to reach targe set aproprite flags
        PLOS_CNT_INC();
        setMAX_RT_IRQ();
        delete lastTransmited;
        lastTransmited = TXpacket;
        waitingForAck = false;
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
        printf("%d: nRF24l01+::startTimer %d\n", id, time);
        theTimer.setStartInterval(time);
        theTimer.setPeriodicInterval(0);
        theTimer.start(*noACKalarmCallback);
    }
}


