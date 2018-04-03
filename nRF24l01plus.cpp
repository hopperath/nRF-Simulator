#include <Poco/Delegate.h>
#include "nRF24l01plus.h"
#include "ether.h"

using namespace std;
using namespace Poco;

//constructor
nRF24l01plus::nRF24l01plus(int id, Ether* someEther) : theEther(someEther), chip(&nRF24l01plus::runRF24,this)
{
    //ctor
    this->id = id;

    //inbound events
    theEther->dispatchMsgEvent += Poco::delegate(this, &nRF24l01plus::receiveMsgFromEther);
    theEther->collisionEvent += Poco::delegate(this, &nRF24l01plus::setCollision);

    //outbound events
    sendMsgEvent += Poco::delegate(someEther, &Ether::enterEther);

    noACKalarmCallback = unique_ptr<TimerCallback<nRF24l01plus>>(new TimerCallback<nRF24l01plus>(*this, &nRF24l01plus::noACKalarm));
}


void nRF24l01plus::receiveMsgFromEther(const void* pSender, tMsgFrame& msg)
{
    //copy from ether
    rxMsg = shared_ptr<tMsgFrame>(new tMsgFrame(msg));

    if (rxMsg->radioId == 0)
    {
        printf("ERROR: radioId=0\n");
    }
    //Sent from this radio, ignore. Software event system sends to all radios.
    //In the real world the sending radio would not hear its own transmission
    if (rxMsg->radioId==id)
    {
        return;
    }

    printf("%d: nRF24l01plus::receiveMsgFromEther msg=%s collision=%d\n", id, rxMsg->toString().c_str(),collision);

    if (!collision)
    {
        //collision did not happen
        cmdNotify(PRX);
    }
    collision = false;
}

void nRF24l01plus::startPRX()
{
    printf("%d: pwrup=%d  ce=%d rx_mode=%d waitingForAck=%d\n", id, isPWRUP(), getCE(), isRX_MODE(), waitingForAck);
    if (waitingForAck)
    {
        //waiting for ack
        byte pipe = addressToPipe(rxMsg->Address);
        printf("%d: addr=0x%llx  ack_addr=0x%llx\n", id, rxMsg->Address,ACK_address);
        if (rxMsg->Address == ACK_address)
        {
            //addess is the P0 address, this is ACK packet
            ackReceived(rxMsg, pipe);
        }
    }
    else if (isPWRUP() && getCE())
    {
        //in a Standby mode (PWRUP = 1 && CE = 1)
        byte pipe = addressToPipe(rxMsg->Address);
        //printf("%d: pipe=%u\n", id, pipe);
        if (isRX_MODE())
        {//receiving
            //check if address is one off the pipe addresses
            if (pipe!=0xFF)
            {//pipe is open ready to receve
                //fill RX buffer1
                receive_frame(rxMsg, pipe);
                sendAutoAck(rxMsg,pipe);
            }
        }
    }
    rxMsg = nullptr;
}

void nRF24l01plus::sendAutoAck(shared_ptr<tMsgFrame> theFrame, byte pipe)
{
    printf("%d: nRF24l01plus::sendAutoAck\n", id);
    if (theFrame->Packet_Control_Field.NO_ACK)
    {
        return;
    }

    auto msg = getAckPacketForPipe(pipe);

    //Ack packet
    if (msg)
    {
        printf("%d: nRF24l01plus::send ackPayload\n", id);
        ackPktFrame = msg;
        ackPktFrame->Packet_Control_Field.NO_ACK = 0;
        setTX_MODE();
        sendMsgToEther(ackPktFrame);
        setRX_MODE();
    }
    else //Simple Ack
    {
        printf("%d: nRF24l01plus::send simpleAck\n", id);
        auto ackFrame = shared_ptr<tMsgFrame>(new tMsgFrame);
        ackFrame->radioId = id;
        ackFrame->Packet_Control_Field.Payload_length = 0;
        ackFrame->Packet_Control_Field.NO_ACK = 0;
        ackFrame->Packet_Control_Field.PID = nextPID();
        ackFrame->Address = theFrame->Address;

        setTX_MODE();
        sendMsgToEther(ackFrame);
        setRX_MODE();
    }
}

void nRF24l01plus::sendMsgToEther(shared_ptr<tMsgFrame> theMSG)
{
    printf("%d: sendMsgToEther:msg.addr=%llx\n",id,theMSG->Address);
    sendMsgEvent.notifyAsync(this, *theMSG);
}

/*
 * Start transmission...
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

    //Does not remove from TX_FIFO, per datasheet
    auto packetToSend = getTXpacket();
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
    {
        //no ack, last transmitted is the one that is
        removeTXPacket(packetToSend);
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
    cmdNotify(PTX);
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
    cmdNotify(PTX);
}

void nRF24l01plus::TXpacketAdded()
{
    if (getCE()==false || isRX_MODE()==true || isPWRUP()==false)
    {
        return;
    }
    cmdNotify(PTX);
}

void nRF24l01plus::PWRUPset()
{
    if (getCE()==false || isRX_MODE()==true)
    {
        return;
    }
    cmdNotify(PTX);
}

void nRF24l01plus::ackReceived(shared_ptr<tMsgFrame> theMSG, byte pipe)
{

    printf("%d: nRF24l01+::ackReceived\n", id);
    if (theMSG->Packet_Control_Field.Payload_length==0)
    {
        printf("%d: nRF24l01+::simpleAck received\n", id);
        //regular ACK received :D
    }
    else
    {
        //could be ACK with payload
       if (isACKPayloadEnabled() && isDynamicPayloadEnabled())
       {
           printf("%d: nRF24l01+::ackPayload received\n", id);
           receive_frame(theMSG, pipe);
       }
    }

    waitingForAck = false;
    theTimer.stop();
    setTX_DS_IRQ();
}

void nRF24l01plus::noACKalarm(Timer& timer)
{
    printf("%d: nRF24l01+::noACKalarm ARC_CNT=%u ARC=%u\n", id,getARC_CNT(),getARC());
    if (getARC_CNT()==getARC())
    {
        //number of max retransmits reached
        //failed to reach targe set appropriate flags
        PLOS_CNT_INC();
        setMAX_RT_IRQ();
        //Still in TX_FIFO
        //Will be sent on next TX unless TX_FLUSH is called
        TXpacket= nullptr;
        waitingForAck = false;
    }
    else
    {//retransmit message and increase reTransCounter and start timer again
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

void nRF24l01plus::runRF24()
{
    printf("%id:running\n",id);
    while (true)
    {
        std::unique_lock<std::mutex> lock(m);
        cmdAvailable.wait(lock);
        processCmd();
        lock.unlock();
    }
}

void nRF24l01plus::processCmd()
{
    printf("%u: processing cmd=%d\n",id,pendingCmd);
    switch(pendingCmd)
    {
        case PTX:
            startPTX();
            break;
        case PRX:
            startPRX();
            break;
    }
    pendingCmd=IDLE;
}

//This will run in the rf24 thread
void nRF24l01plus::cmdNotify(int cmd)
{
    if (pendingCmd==IDLE)
    {
        pendingCmd = cmd;
        cmdAvailable.notify_one();
    }
    else
    {
        printf("%u: cmd already pending cmd=%d",id,pendingCmd);
        return;
    }
}


