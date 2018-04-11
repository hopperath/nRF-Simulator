#include "nRF24interface.h"
#include <stdio.h>

using namespace std;

nRF24interface::nRF24interface() : nRF24registers(), PID(0)
{
    //ctor
}

commands nRF24interface::get_command(byte command)
{
    switch (command)
    {
        case 0b11111111://NOP
            return eNOP;
        case 0b01100001://R_RX_PAYLOAD
            return eR_RX_PAYLOAD;
        case 0b10100000://W_TX_PAYLOAD
            return eW_TX_PAYLOAD;
        case 0b11100001://FLUSH_TX
            return eFLUSH_TX;
        case 0b11100010://FLUSH_RX
            return eFLUSH_RX;
        case 0b11100011://REUSE_TX_PL
            return eREUSE_TX_PL;
        case 0b01100000://R_RX_PL_WID
            return eR_RX_PL_WID;
        case 0b10110000://W_TX_PAYLOAD_NO_ACK
            return eW_TX_PAYLOAD_NO_ACK;
        default:
            break;
    }

    byte three_most_significant_bits = command >> 5;
    switch (three_most_significant_bits)
    {
        case 0b000://R_REGISTER
            return eR_REGISTER;
        case 0b001://W_REGISTER
            return eW_REGISTER;
        case 0b101://W_ACK_PAYLOAD
            return eW_ACK_PAYLOAD;
        default:
            return eNOP;
    }
}

byte nRF24interface::Spi_Write(byte* msg, int spiMsgLen, byte* dataBack, int dataMax)
{
    //Original STATUS is returned on every command
    byte statusCMD = eSTATUS;
    byte status = *read_register(&statusCMD);

    //identify the command from the first byte
    commands theCommand = get_command(*msg);
    byte addr = *msg&0b00011111;
    shared_ptr<tMsgFrame> tempMsgFrame = nullptr;

    //printf("COMMAND SENT: 0x%x\n",*msg);
    switch (theCommand)
    {

        case eW_REGISTER:
            //printf("COMMAND SENT: W_REGISTER\n");
            write_register(msg);
            break;
        case eR_REGISTER:
            //TODO: This should check datamax
            //printf("\n\nCOMMAND SENT: R_REGISTER");
            byte* read_reg;
            read_reg = read_register(msg); //load the register into temp read_reg
            if (read_reg==nullptr)
            {
                break;
            }
            *dataBack = *read_reg;
            //printf("\nREAD: 0x%X",msgBack[0]);
            if ((addr==eRX_ADDR_P0) || (addr==eRX_ADDR_P1) || addr==eTX_ADDR)
            {
                *((uint64_t*) (dataBack)) = *((uint64_t*) read_reg);
                //printf("\n0x%llX\n",*((uint64_t*)msgBack));
            }
            break;
        case eR_RX_PAYLOAD:
            printf("%s COMMAND SENT: R_RX_PAYLOAD\n",LOGHDR);
            tempMsgFrame = read_RX_payload();
            if (tempMsgFrame!=nullptr)
            {
                if (tempMsgFrame->Packet_Control_Field.Payload_length<=dataMax)
                {
                    memcpy(dataBack, tempMsgFrame->Payload, tempMsgFrame->Packet_Control_Field.Payload_length);
                }
                else
                {
                    memcpy(dataBack, tempMsgFrame->Payload, dataMax);
                }
            }
            break;

        case eW_TX_PAYLOAD:
            printf("%s W_TX_PAYLOAD\n",LOGHDR);
            write_TX_payload(msg + 1, spiMsgLen);
            break;
        case eFLUSH_TX:
            //printf("%s COMMAND SENT: FLUSH_TX\n",LOGHDR);
            flush_tx();
            break;
        case eFLUSH_RX:
            //printf("%s COMMAND SENT: FLUSH_RX\n",LOGHDR);
            flush_rx();
            break;
        case eR_RX_PL_WID:
            printf("%s COMMAND SENT: R_RX_PL_WID\n",LOGHDR);
            dataBack[0] = read_RX_payload_width();
            break;
        case eW_ACK_PAYLOAD:
            printf("%s COMMAND SENT: W_ACK_PAYLOAD\n",LOGHDR);
            write_ack_payload(msg + 1, spiMsgLen);
            break;
        case eW_TX_PAYLOAD_NO_ACK:
            printf("%s COMMAND SENT: W_TX_PAYLOAD_NO_ACK\n",LOGHDR);
            write_no_ack_payload(msg + 1, spiMsgLen);
            break;
        case eNOP:
        default:
            break;
    }

    //printf("%s STATUS: " BYTE_TO_BINARY_PATTERN "\n",LOGHDR,BYTE_TO_BINARY(status));
    /*
    byte reg = eCONFIG;
    printf("  CONFIG: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*read_register(&reg)));
    reg = eFIFO_STATUS;
    printf("  FIFO: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*read_register(&reg)));
     */

    return status;
}

uint8_t nRF24interface::read_RX_payload_width()
{
    if (isFIFO_RX_EMTPY())
    {
        return 0;
    }

    lock_guard<mutex> lock_rx_fifo(rx_mutex);

    if (RX_FIFO.empty())
    {
        return 0;
    }
    else
    {
        return RX_FIFO.front()->Packet_Control_Field.Payload_length;
    }
}

void nRF24interface::removeTXPacket(shared_ptr<tMsgFrame> msgFrame)
{
    shared_ptr<tMsgFrame> temp;

    lock_guard<mutex> lock(tx_mutex);

    auto sizeOfTXfifo = TX_FIFO.size();
    int i = 0;

    while (i++<sizeOfTXfifo)
    {
        if (TX_FIFO.front()==msgFrame)
        {
            TX_FIFO.pop();
        }
        /*
        if (TX_FIFO.front()==msgFrame)
        {
            TX_FIFO.pop();

            if (i==1)
            {//arrange back TX_FIFO
                temp = TX_FIFO.front();
                TX_FIFO.pop();
                TX_FIFO.push(temp);
            }
            //if tx fifo was full size = 3
            //remove full flag
            if (sizeOfTXfifo==3)
            {
                clearTX_FULL_IRQ();
                clearTX_FULL();
            }
            //if tx fifo only had one size = 1
            //set the tx fifo empty flag
            if (sizeOfTXfifo==1)
            {
                setTX_EMPTY();
            }
        }
         */

        //Push to back
        if (!TX_FIFO.empty())
        {
            temp = TX_FIFO.front();
            TX_FIFO.pop();
            TX_FIFO.push(temp);
        }
    }


    if (TX_FIFO.size()==0)
    {
        setTX_EMPTY();
    }

    if (TX_FIFO.size()<3 && sizeOfTXfifo==3)
    {
        clearTX_FULL_IRQ();
        clearTX_FULL();
    }
}

shared_ptr<tMsgFrame> nRF24interface::getTXpacket(uint64_t address)
{
    printf("%s getTXPacket looking for addr=0x%llx\n",LOGHDR, address);
    lock_guard<mutex> lock(tx_mutex);
    if (TX_FIFO.empty()==true)
    {
        return nullptr; //nothing to send
    }

    int i = 0;
    auto sizeOfTXfifo = static_cast<int>(TX_FIFO.size());

    shared_ptr<tMsgFrame> temp;
    while (i++<sizeOfTXfifo)
    {
        if (TX_FIFO.front()->Address==address)
        {
            printf("%s getTXPacket found addr=0x%llx\n",LOGHDR, address);
            return TX_FIFO.front();
        }
        temp = TX_FIFO.front();
        TX_FIFO.pop();
        TX_FIFO.push(temp);
    }

    return nullptr;
}

shared_ptr<tMsgFrame> nRF24interface::read_RX_payload()
{
    if (isPRIM_RX() || (isACKPayloadEnabled() && isDynamicPayloadEnabled()))
    {
        lock_guard<mutex> lock(rx_mutex);

        if (RX_FIFO.empty())
        {
            return nullptr;
        }

        //Not empty
        shared_ptr<tMsgFrame> front = RX_FIFO.front();
        RX_FIFO.pop();

        if (RX_FIFO.empty())
        {
            setRX_EMPTY();
            setRX_P_NO(7);//empty
        }
        else
        {
            //still has frames in FIFO change waiting pipe
            auto temp = RX_FIFO.front();
            setRX_P_NO(addressToPipe(temp->Address));
        }
        clearRX_FULL();
        return front;
    }
    return nullptr;
}

void nRF24interface::newFrame(uint64_t Address, uint8_t PayLength, uint8_t pid, uint8_t noAckFlag, uint8_t* Payload)
{
    lock_guard<mutex> lock(tx_mutex);

    auto theFrame = shared_ptr<tMsgFrame>(new tMsgFrame);
    int i = 0;
    while ((i<(PayLength)) && (i<32))
    {
        //writes all the bytes into the payload
        theFrame->Payload[i] = Payload[i];
        i++;
    }
    //Set Radio id to ignore from ether
    theFrame->radioId = id;
    //Set PCF payload length
    theFrame->Packet_Control_Field.Payload_length = static_cast<uint8_t>(i);
    //Set NO_ACK flag to zero (request ACK);
    theFrame->Packet_Control_Field.NO_ACK = noAckFlag;
    //Set PID                            //76543210
    theFrame->Packet_Control_Field.PID = pid;
    //Set Address
    theFrame->Address = Address;

    TX_FIFO.push(theFrame);

    printf("%s newFrame push addr=0x%llx, len=%u, pid=%u, noAck=%u\n",LOGHDR, Address, PayLength, pid, noAckFlag);
    clearTX_EMPTY();
    if (TX_FIFO.size()==3)
    {
        setTX_FULL();
        setTX_FULL_IRQ();
    }
    //Not an ACK payload
    if (theFrame->Address==0)
    {
        TXPacketAdded();
    }
}

uint8_t nRF24interface::nextPID()
{
    return static_cast<uint8_t>(0b00000011)&PID++;
}

void nRF24interface::write_TX_payload(byte* bytes_to_write, int len)
{
    if (isFIFO_TX_FULL())
    {
        return;
    }
    newFrame(0, len, nextPID(), 0, bytes_to_write);
}

void nRF24interface::write_no_ack_payload(byte* bytes_to_write, int len)
{
    if (!isDynamicACKEnabled() || isFIFO_RX_FULL())
    {
        return;
    }
    newFrame(0, len, nextPID(), 1, bytes_to_write);
}

void nRF24interface::write_ack_payload(byte* bytes_to_write, int len)
{
    //check if tx fifo is full
    if (isFIFO_TX_FULL()==1)
    {
        return;
    }
    //check if Dynamic Payload is enabled
    if (isDynamicPayloadEnabled()==0)
    {
        return;
    }

    byte pipe = bytes_to_write[0]&static_cast<uint8_t>(0b00000111); //3 lsb bist are the pype;

    uint64_t ACK_address = getAddressFromPipe(pipe);
    if (ACK_address==0)
    {
        printf("%s No address for pipe %u",LOGHDR, pipe);
        return;
    }

    newFrame(ACK_address, len, 0, 1, bytes_to_write);
}

void nRF24interface::flush_rx()
{
/*****check if device is in RX mode*/
    if (isPRIM_RX()==1)
    {
        lock_guard<mutex> lock(rx_mutex);
        while (!RX_FIFO.empty())
        {
            auto temp = RX_FIFO.front();
            RX_FIFO.pop();
        }
        setRX_EMPTY();
        clearRX_FULL();
    }
}

void nRF24interface::flush_tx()
{
    /********chec if device is in TX mode*******************/
    if (isPRIM_RX()==0)
    {
        lock_guard<mutex> lock(tx_mutex);
        while (!TX_FIFO.empty())
        {
            TX_FIFO.pop();
        }
        clearTX_FULL();
        setTX_EMPTY();
        clearTX_FULL_IRQ();
    }
}

shared_ptr<tMsgFrame> nRF24interface::getAckPacketForPipe(uint8_t pipe)
{

    printf("%s nRF24interface::getAckPacketForPipe pipe=%u DYN_PAY=%u ACK_PAY=%u\n",LOGHDR, pipe,
           isDynamicPayloadEnabled(), isACKPayloadEnabled());
    //feature not enabled
    if (isDynamicPayloadEnabled() && isACKPayloadEnabled())
    {
        uint64_t pipe_address = getAddressFromPipe_ENAA(pipe);
        printf("%s nRF24interface::pipe addr=0x%llx\n",LOGHDR, pipe_address);
        if (pipe_address==0)
        {
            return nullptr;
        }

        return getTXpacket(pipe_address);
    }
    return nullptr;
}

bool nRF24interface::receive_frame(shared_ptr<tMsgFrame> theFrame, byte pipe)
{
    printf("%s nRF24interface::receive_frame pipe=%u\n",LOGHDR, pipe);


    //Check for duplicate
    if (lastReceived)
    {
        if (lastReceived->Address==theFrame->Address &&
            lastReceived->Packet_Control_Field.PID==theFrame->Packet_Control_Field.PID &&
            lastReceived->Packet_Control_Field.Payload_length==theFrame->Packet_Control_Field.Payload_length &&
            memcmp(lastReceived->Payload, theFrame->Payload, lastReceived->Packet_Control_Field.Payload_length)==0
                )
        {
            //Dupe
            printf("%s nRF24interface::dupe frame receive_frame pipe=%u\n",LOGHDR, pipe);
            return false;
        }
        lastReceived = theFrame;
    }


    /*********check if buffer is full*******/
    if (isFIFO_RX_FULL())
    {
        return false;
    }

    /***Receive the frame***/
    /********push into RX FIFO*******/
    if (isFIFO_RX_EMTPY())
    {
        setRX_P_NO(pipe);
    }
    printf("%s nRF24interface::push rx_frame\n",LOGHDR);
    lock_guard<mutex> lock(rx_mutex);
    RX_FIFO.push(theFrame);

    if (RX_FIFO.size()==3)
    {
        setRX_FULL();
    }

    setRX_DR_IRQ();//issue irq...
    clearRX_EMPTY();

    return true;
}

uint16_t nRF24interface::crc16(const unsigned char* data_p, uint8_t length)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--)
    {
        x = crc >> 8^*data_p++;
        x ^= x >> 4;
        crc = (crc << 8)^((uint16_t) (x << 12))^((uint16_t) (x << 5))^((uint16_t) x);
    }
    return crc;
}

