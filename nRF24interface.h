#ifndef NRF24INTERFACE_H
#define NRF24INTERFACE_H

#include "nRF24registers.h"
#include <queue>
#include "msgframe.h"

class nRF24interface : public nRF24registers
{
    protected:
        std::shared_ptr<tMsgFrame> lastReceived;

    private:
        //interface registers
        std::queue<std::shared_ptr<tMsgFrame>> RX_FIFO;
        std::queue<std::shared_ptr<tMsgFrame>> TX_FIFO;
        uint8_t PID;

    public:
        /** Default constructor */
        nRF24interface();

        byte Spi_Write(byte * msg, int spiMsgLen, byte* dataBack, int dataMax=0);
        //move to protected
        bool receive_frame(std::shared_ptr<tMsgFrame> theFrame, byte pipe);

    protected:
        //inteface functions
        void newFrame(uint64_t Address, uint8_t PayLength, uint8_t thePID, uint8_t theNP_ACK,uint8_t* Payload);
        std::shared_ptr<tMsgFrame> read_RX_payload();
        std::shared_ptr<tMsgFrame> getTXpacket(uint64_t address = 0);
        void write_TX_payload(byte* bytes_to_write, int len);
        void write_ack_payload(byte* bytes_to_write, int len);
        void write_no_ack_payload(byte* bytes_to_write, int len);
        std::shared_ptr<tMsgFrame> getAckPacketForPipe(uint8_t pipe);
        void flush_tx();
        void flush_rx();
        uint8_t read_RX_payload_width();
        commands get_command(byte command);
        uint8_t nextPID();
        void removeTXPacket(std::shared_ptr<tMsgFrame> msgFrame);
        uint16_t crc16(const unsigned char* data_p, uint8_t length);


};

#endif // NRF24INTERFACE_H
