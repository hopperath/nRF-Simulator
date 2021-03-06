#ifndef NRF24REGISTERS_H
#define NRF24REGISTERS_H

#include <stdint.h>
#include <string>
#include "nRF24bits_struct.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')


#define LOGHDR logHdr().c_str()

class nRF24registers
{
    public:
        //Radio id for our use
        //does not exist in chip
        int id;

    public:
        /** Default constructor */
        nRF24registers();
        /** Default destructor */
        virtual ~nRF24registers();
        void printRegContents();
        bool checkIRQ();
        void setCE_HIGH();
        void setCE_LOW();
        bool getCE(){return CE;}

    protected:
        byte * read_register(byte * read_command);
        void write_register(byte * bytes_to_write);
        byte addressToPipe(uint64_t address);
        uint64_t getAddressFromPipe(byte pipe);
        uint64_t getAddressFromPipe_ENAA(byte pipe);
        uint8_t getARD(){return REGISTERS.sSETUP_RETR.sARD;}
        uint8_t getARC(){return REGISTERS.sSETUP_RETR.sARC;}
        uint64_t getTXaddress();
        uint8_t getENAA(byte pipe);
        bool isACKPayloadEnabled(){return REGISTERS.sFEATURE.sEN_ACK_PAY;}
        bool isDynamicACKEnabled(){return REGISTERS.sFEATURE.sEN_DYN_ACK;}
        bool isDynamicPayloadEnabled(){return REGISTERS.sFEATURE.sEN_DPL;}
        bool isFIFO_TX_EMPTY(){return REGISTERS.sFIFO_STATUS.sTX_EMPTY;}
        bool isFIFO_TX_FULL(){return REGISTERS.sFIFO_STATUS.sTX_FULL;}
        bool isFIFO_RX_EMTPY(){return REGISTERS.sFIFO_STATUS.sRX_EMPTY;}
        bool isFIFO_RX_FULL(){return REGISTERS.sFIFO_STATUS.sRX_FULL;}
        bool isPRIM_RX(){return REGISTERS.sCONFIG.sPRIM_RX;}
        void _setTX_MODE(){REGISTERS.sCONFIG.sPRIM_RX = 0;}
        void _setRX_MODE(){REGISTERS.sCONFIG.sPRIM_RX = 1;}
        bool isPWRUP(){return REGISTERS.sCONFIG.sPWR_UP;}
        void clearRX_FULL(){REGISTERS.sFIFO_STATUS.sRX_FULL = 0;}
        void setRX_FULL(){REGISTERS.sFIFO_STATUS.sRX_FULL = 1;}
        void clearRX_EMPTY(){REGISTERS.sFIFO_STATUS.sRX_EMPTY = 0;}
        void setRX_EMPTY(){REGISTERS.sFIFO_STATUS.sRX_EMPTY = 1;}
        void setTX_FULL();
        void clearTX_FULL();
        void setTX_EMPTY(){REGISTERS.sFIFO_STATUS.sTX_EMPTY = 1;}
        void clearTX_EMPTY(){REGISTERS.sFIFO_STATUS.sTX_EMPTY = 0;}
        void setRX_DR(){REGISTERS.sSTATUS.sRX_DR = 1;}
        void setMAX_RT(){REGISTERS.sSTATUS.sMAX_RT =1;}
        void setTX_DS(){REGISTERS.sSTATUS.sTX_DS = 1;}
        void PLOS_CNT_INC();
        void clearPLOS_CNT(){REGISTERS.sOBSERVE_TX.sPLOS_CNT = 0;}
        void ARC_CNT_INC();
        void clearARC_CNT(){REGISTERS.sOBSERVE_TX.sARC_CNT = 0;}
        uint8_t getARC_CNT(){return REGISTERS.sOBSERVE_TX.sARC_CNT;}
        void setRX_P_NO(byte pipe){REGISTERS.sSTATUS.sRX_P_NO = pipe;}


    private:
        tREGISTERS REGISTERS;
        void* register_array[0x1E];
        bool CE;

    protected:
    //signals
        virtual void CEset() = 0;
        virtual void RXTXmodeSet() = 0;
        virtual void PWRset() = 0;
        virtual void TXPacketAdded() = 0;
        virtual std::string logHdr() = 0;
};

#endif // NRF24REGISTERS_H
