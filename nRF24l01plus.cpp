#include <Poco/Delegate.h>
#include <iomanip>
#include "nRF24l01plus.h"
#include "ether.h"
#include "ThreadNames.h"

using namespace std;
using namespace Poco;

//constructor
nRF24l01plus::nRF24l01plus(int id, Ether* someEther, MCUClock& clock) : theEther(someEther), chip(&nRF24l01plus::runRF24, this), mcuClock(clock)
{
    //ctor
    this->id = id;

    ThreadNames::setName(chip.get_id(),string("rf")+to_string(id));

    printf("%s %s\n", LOGHDR, stateToString(radioState));
    //inbound events
    theEther->dispatchMsgEvent += Poco::delegate(this, &nRF24l01plus::receiveMsgFromEther);
    theEther->collisionEvent += Poco::delegate(this, &nRF24l01plus::setCollision);

    //outbound events
    sendMsgEvent += Poco::delegate(someEther, &Ether::enterEther);

    noACKalarmCallback = unique_ptr<TimerCallback<nRF24l01plus>>(new TimerCallback<nRF24l01plus>(*this, &nRF24l01plus::noACKalarm));
}

string nRF24l01plus::logHdr()
{
    ostringstream hdr;
    //hdr << id << ": " << millis() << ": t" << this_thread::get_id() <<": ";
    hdr << setw(2) << id << ": " << setw(5) << millis() << ":" <<setw(5)<< ThreadNames::getName() << ":";
    return hdr.str();
}

uint32_t nRF24l01plus::millis()
{
    return mcuClock.millis();
}


void nRF24l01plus::receiveMsgFromEther(const void* pSender, tMsgFrame& msg)
{
    //printf("%s nRF24l01plus::receiveMsgFromEther top\n", LOGHDR);
    //copy from ether
    rxMsg = shared_ptr<tMsgFrame>(new tMsgFrame(msg));

    if (rxMsg->radioId<0)
    {
        printf("ERROR: radioId<0\n");
    }
    //Sent from this radio, ignore. Software event system sends to all radios.
    //In the real world the sending radio would not hear its own transmission
    if (rxMsg->radioId==id)
    {
        return;
    }

    printf("%s nRF24l01plus::receiveMsgFromEther msg=%s collision=%d mode=%s %s\n", LOGHDR, rxMsg->toString().c_str(), collision,stateToString(radioState),(radioState==S_RX_MODE)?"":"ignored");

    if (radioState!=S_RX_MODE)
    {
        return;
    }

    if (!collision)
    {
        //collision did not happen
        cmdNotify(PRX);
    }
    collision = false;
}

//This only gets called in S_RX_MODE
void nRF24l01plus::startPRX()
{
    printf("%s pwrup=%d  ce=%d rx_mode=%d waitingForAck=%d ackAddr=0x%llx\n", LOGHDR, isPWRUP(), getCE(), radioState==S_RX_MODE, waitingForAck, ACK_address);
    if (waitingForAck)
    {
        //waiting for ack
        byte pipe = addressToPipe(rxMsg->Address);
        printf("%s addr=0x%llx  ack_addr=0x%llx\n", LOGHDR, rxMsg->Address, ACK_address);
        if (rxMsg->Address==ACK_address)
        {
            //addess is the P0 address, this is ACK packet
            ackReceived(rxMsg, pipe);
        }
    }
    else
    {
        byte pipe = addressToPipe(rxMsg->Address);
        printf("%s addr=0x%llx pipe=%d %s\n", LOGHDR, rxMsg->Address, pipe, (pipe==0xFF)?"ignoring":"");
        //check if address is one off the pipe addresses
        if (pipe!=0xFF)
        {
            //pipe is open ready to receive
            //fill RX buffer
            receive_frame(rxMsg, pipe);
            sendAutoAck(rxMsg, pipe);
        }
        /*
        else
        {
            printf("%s ignoring pkt from 0x%llx\n",id,rxMsg->Address);
        }
         */
    }
    rxMsg = nullptr;
}

void nRF24l01plus::sendAutoAck(shared_ptr<tMsgFrame> theFrame, byte pipe)
{
    if (theFrame->Packet_Control_Field.NO_ACK || !getENAA(pipe))
    {
        return;
    }

    printf("%s nRF24l01plus::sendAutoAck\n", LOGHDR);

    auto msg = getAckPacketForPipe(pipe);

    //Ack packet
    if (msg)
    {
        printf("%s nRF24l01plus::send ackPayload\n", LOGHDR);
        ackPktFrame = msg;
        ackPktFrame->Packet_Control_Field.NO_ACK = 0;
        setTX_MODE();
        sendMsgToEther(ackPktFrame);
        setRX_MODE();
    }
    else //Simple Ack
    {
        printf("%s nRF24l01plus::send simpleAck\n", LOGHDR);
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

//This happens internally
void nRF24l01plus::setRX_MODE()
{
    _setRX_MODE();
    setRadioState(S_RX_MODE);
}

//This happens internally
void nRF24l01plus::setTX_MODE()
{
    _setTX_MODE();
    setRadioState(S_TX_MODE);
}

void nRF24l01plus::sendMsgToEther(shared_ptr<tMsgFrame> theMSG)
{
    printf("%s sendMsgToEther:msg.addr=%llx\n", LOGHDR, theMSG->Address);
    sendMsgEvent.notifyAsync(this, *theMSG);
}

/*
 * Start transmission...
 *
 */
void nRF24l01plus::startPTX()
{
    printf("%s nRF24l01plus::startPTX\n", LOGHDR);

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
    if ((packetToSend->Packet_Control_Field.NO_ACK==0) && (getARC()!=0) && getENAA(0))
    {
        //set for ACK recipt..
        ACK_address = getTXaddress();
        waitingForAck = true;
        clearARC_CNT();
        printf("%s Autoretry delay ARD=%u\n", LOGHDR, getARD());
        setRX_MODE();
        startTimer(getARD() * ACK_TIMEOUT_FACTOR);
        TXpacket = packetToSend;
    }
    else
    {
        //no ack, last transmitted is the one that is
        removeTXPacket(packetToSend);
        standbyTransition();
        setTX_DS_IRQ();
    }
}

/*
 * in PTX if CE is set HIGH and there is a packet waiting transmit packet
 * this only changes state based on reading register values. Ok on MCU thread.
 */
void nRF24l01plus::CEset()
{
    printf("%s CEset CE=%d state=%s\n", LOGHDR, getCE(),stateToString());

    //TODO: this should be a "shockburst mode" flag.
    //Ignore CE if processing
    if (waitingForAck==false)
    {
        if (radioState==S_STANDBY_I)
        {
            standbyTransition();
        }
        else if (radioState!=S_PWRDOWN) //All other paths lead to STANDBY_I with CE=0
        {
            setRadioState(S_STANDBY_I);
        }
    }
}

/*
 * If TX mode is set while CE is high and there is a packet waiting transmit packet
 *
 */
void nRF24l01plus::RXTXmodeSet()
{
    printf("%s RXTXmodeSet PRIM_RX=%d state=%s\n", LOGHDR, isPRIM_RX(), stateToString());
    //Only changes if in standby_1 i think...
    if (radioState==S_STANDBY_I)
    {
        standbyTransition();
    }
}

void nRF24l01plus::TXPacketAdded()
{
    printf("%s TXPacketAdded state=%s\n", LOGHDR, stateToString());
    if (radioState==S_STANDBY_II)
    {
        setRadioState(S_TX_MODE);
        cmdNotify(PTX);
    }
}

void nRF24l01plus::PWRset()
{
    printf("%s PWRset PWR=%d state=%s\n", LOGHDR, isPWRUP(), stateToString());
    if (isPWRUP())
    {
        setRadioState(S_STANDBY_I);
    }
    else
    {
        setRadioState(S_PWRDOWN);
        return;
    }

    //If CE set do another state transition
    standbyTransition();
}

void nRF24l01plus::setRadioState(int state)
{
    switch (state)
    {
        case S_PWRDOWN:
        case S_STANDBY_I:
        case S_STANDBY_II:
        case S_RX_MODE:
        case S_TX_MODE:
            if (radioState!=state)
            {
                printf("%s %s -> %s\n", LOGHDR, stateToString(radioState), stateToString(state));
            }
            radioState = state;
            break;

        default:
            printf("%s bad radio state=%d!", LOGHDR, state);
    }
}

const char* nRF24l01plus::stateToString()
{
    return stateToString(radioState);
}
const char* nRF24l01plus::stateToString(int state)
{
    switch (state)
    {
        case S_PWRDOWN:
            return "PWRDOWN";
        case S_STANDBY_I:
            return "STANDBY_I";
        case S_STANDBY_II:
            return "STANDBY_II";
        case S_RX_MODE:
            return "RX_MODE";
        case S_TX_MODE:
            return "TX_MODE";
        default:
            return "UNKNOWN";
    }
}

void nRF24l01plus::ackReceived(shared_ptr<tMsgFrame> theMSG, byte pipe)
{

    printf("%s nRF24l01+::ackReceived\n", LOGHDR);
    if (theMSG->Packet_Control_Field.Payload_length==0)
    {
        printf("%s nRF24l01+::simpleAck received\n", LOGHDR);
        //regular ACK received :D
    }
    else
    {
        //could be ACK with payload
        if (isACKPayloadEnabled() && isDynamicPayloadEnabled())
        {
            printf("%s nRF24l01+::ackPayload received\n", LOGHDR);
            receive_frame(theMSG, pipe);
        }
    }
    removeTXPacket(TXpacket);
    setTX_MODE();
    standbyTransition();
    waitingForAck = false;
    theTimer->stop();
    setTX_DS_IRQ();
}

void nRF24l01plus::removeTXPacket(shared_ptr<tMsgFrame> frame)
{
    nRF24interface::removeTXPacket(frame);
}

//TODO: Go to proper state after trying all items? does it try them all? in FIFO, i.e. CE=1
void nRF24l01plus::standbyTransition()
{
    printf("%s PWR=%d PRIM_RX=%d CE=%d isFIFO_TX_EMPTY=%u\n", LOGHDR,isPWRUP(),isPRIM_RX(),getCE(),isFIFO_TX_EMPTY());

    if (getCE())
    {
        if (isPRIM_RX())
        {
            setRadioState(S_RX_MODE);
        }
        else // PRIM_RX=0 PTX
        {
            if (isFIFO_TX_EMPTY())
            {
                setRadioState(S_STANDBY_II);
            }
            else
            {
                setRadioState(S_TX_MODE);
                cmdNotify(PTX);
            }
        }
    }
    else
    {
        setRadioState(S_STANDBY_I);
    }
}

void nRF24l01plus::noACKalarm(Poco::Timer& timer)
{
    cmdNotify(PNOACK) ;
}

void nRF24l01plus::processNoAck()
{
    printf("%s nRF24l01+::noACKalarm ARC_CNT=%u ARC=%u\n", LOGHDR, getARC_CNT(), getARC());
    if (getARC_CNT()==getARC())
    {
        //number of max retransmits reached
        //failed to reach targe set appropriate flags
        PLOS_CNT_INC();
        setMAX_RT_IRQ();
        //Still in TX_FIFO
        //Will be sent on next TX unless TX_FLUSH is called
        TXpacket = nullptr;
        waitingForAck = false;
        standbyTransition();
    }
    else
    {
        //retransmit message and increase reTransCounter and start timer again
        setTX_MODE();
        sendMsgToEther(TXpacket);
        setRX_MODE();
        //setup wait for ack...
        ARC_CNT_INC();
        startTimer(getARD() * ACK_TIMEOUT_FACTOR);//10ms real life 250us
    }
}

void nRF24l01plus::setCollision()
{
    collision = true;
}

void nRF24l01plus::startTimer(int time)
{
    printf("%s nRF24l01+::startAckTimer %d\n", LOGHDR, time);
    if (time>0)
    {

        printf("%s nRF24l01+::creating  timer\n", LOGHDR);

        theTimer = unique_ptr<Timer>(new Timer(time,0));
        theTimer->start(*noACKalarmCallback);
    }
}

void nRF24l01plus::runRF24()
{
    printf("%s running\n", LOGHDR);
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
    printf("%s processing cmd=%s\n", LOGHDR, cmdToString(pendingCmd));
    switch (pendingCmd)
    {
        case PTX:
            if (radioState==S_TX_MODE)
            {
                startPTX();
            }
            else
            {
                printf("%s not in TX_MODE mode=%s\n", LOGHDR, stateToString(radioState));
            }
            break;
        case PRX:
            if (radioState==S_RX_MODE)
            {
                startPRX();
            }
            else
            {
                printf("%s not in RX_MODE mode=%s\n", LOGHDR, stateToString(radioState));
            }
            break;
        case PNOACK:
            processNoAck();
            break;
    }
    pendingCmd = IDLE;
}

const char* nRF24l01plus::cmdToString(int cmd)
{
    switch (cmd)
    {
        case IDLE:
            return "IDLE";
        case PTX:
            return "PTX";
        case PRX:
            return "PRX";
        case PNOACK:
            return "PNOACK";
        default:
            return "UNKNOWN";
    }
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
        printf("%s cmd already pending cmd=%d", LOGHDR, pendingCmd);
        return;
    }
}


