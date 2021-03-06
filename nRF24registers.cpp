#include "nRF24registers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

nRF24registers::nRF24registers() : CE(false)
{
    register_array[eCONFIG] = &REGISTERS.sCONFIG;
    *((byte*) register_array[eCONFIG]) = 0b00001000;

    register_array[eEN_AA] = &REGISTERS.sEN_AA;
    *((byte*) register_array[eEN_AA]) = 0b00111111;

    register_array[eEN_RXADDR] = &REGISTERS.sEN_RXADDR;
    *((byte*) register_array[eEN_RXADDR]) = 0b00000011;

    register_array[eSETUP_AW] = &REGISTERS.sSETUP_AW;
    *((byte*) register_array[eSETUP_AW]) = 0b00000011;

    register_array[eSETUP_RETR] = &REGISTERS.sSETUP_RETR;
    *((byte*) register_array[eSETUP_RETR]) = 0b00000011;

    register_array[eRF_CH] = &REGISTERS.sRF_CH;
    *((byte*) register_array[eRF_CH]) = 0b00000010;

    register_array[eRF_SETUP] = &REGISTERS.sRF_SETUP;
    *((byte*) register_array[eRF_SETUP]) = 0b00001110;

    register_array[eSTATUS] = &REGISTERS.sSTATUS;
    *((byte*) register_array[eSTATUS]) = 0b00001110;

    register_array[eOBSERVE_TX] = &REGISTERS.sOBSERVE_TX;
    *((byte*) register_array[eOBSERVE_TX]) = 0b00000000;

    register_array[eRPD] = &REGISTERS.sRPD;
    *((byte*) register_array[eRPD]) = 0b00000000;

    register_array[eRX_ADDR_P0] = (uint64_t*) malloc(sizeof(uint64_t));
    *((uint64_t*) register_array[eRX_ADDR_P0]) = 0xE7E7E7E7E7;

    register_array[eRX_ADDR_P1] = (uint64_t*) malloc(sizeof(uint64_t));
    *((uint64_t*) register_array[eRX_ADDR_P1]) = 0xC2C2C2C2C2;

    register_array[eRX_ADDR_P2] = (byte*) malloc(sizeof(byte));
    *((byte*) register_array[eRX_ADDR_P2]) = 0xC3;

    register_array[eRX_ADDR_P3] = (byte*) malloc(sizeof(byte));
    *((byte*) register_array[eRX_ADDR_P3]) = 0xC4;

    register_array[eRX_ADDR_P4] = (byte*) malloc(sizeof(byte));
    *((byte*) register_array[eRX_ADDR_P4]) = 0xC5;

    register_array[eRX_ADDR_P5] = (byte*) malloc(sizeof(byte));
    *((byte*) register_array[eRX_ADDR_P5]) = 0xC6;

    register_array[eTX_ADDR] = (uint64_t*) malloc(sizeof(uint64_t));
    *((uint64_t*) register_array[eTX_ADDR]) = 0xE7E7E7E7E7;

    register_array[eRX_PW_P0] = &REGISTERS.sRX_PW_P0;
    *((byte*) register_array[eRX_PW_P0]) = 0b00000000;

    register_array[eRX_PW_P1] = &REGISTERS.sRX_PW_P1;
    *((byte*) register_array[eRX_PW_P1]) = 0b00000000;

    register_array[eRX_PW_P2] = &REGISTERS.sRX_PW_P2;
    *((byte*) register_array[eRX_PW_P2]) = 0b00000000;

    register_array[eRX_PW_P3] = &REGISTERS.sRX_PW_P3;
    *((byte*) register_array[eRX_PW_P3]) = 0b00000000;

    register_array[eRX_PW_P4] = &REGISTERS.sRX_PW_P4;
    *((byte*) register_array[eRX_PW_P4]) = 0b00000000;

    register_array[eRX_PW_P5] = &REGISTERS.sRX_PW_P5;
    *((byte*) register_array[eRX_PW_P5]) = 0b00000000;

    register_array[eFIFO_STATUS] = &REGISTERS.sFIFO_STATUS;
    *((byte*) register_array[eFIFO_STATUS]) = 0b00010001;

    register_array[0x18] = NULL;
    register_array[0x19] = NULL;
    register_array[0x1A] = NULL;
    register_array[0x1B] = NULL;

    register_array[eDYNPD] = &REGISTERS.sDYNPD;
    *((byte*) register_array[eDYNPD]) = 0b00000000;

    register_array[eFEATURE] = &REGISTERS.sFEATURE;
    *((byte*) register_array[eFEATURE]) = 0b00000000;
}
void nRF24registers::setTX_FULL()
{
    REGISTERS.sFIFO_STATUS.sTX_FULL = 1;
    REGISTERS.sSTATUS.sTX_FULL = 1;
}

void nRF24registers::clearTX_FULL()
{
    REGISTERS.sFIFO_STATUS.sTX_FULL = 0;
    REGISTERS.sSTATUS.sTX_FULL = 0;
}

nRF24registers::~nRF24registers()
{
    //dtor
    for (int i = eRX_ADDR_P0; i<=eTX_ADDR; i++)
    {
        free(register_array[i]);
    }
}

void nRF24registers::setCE_HIGH()
{
    printf("%s setCE_HIGH CE=%d\n", LOGHDR, CE);

    if (!CE)
    {
        CE = true;
        //was signal
        CEset();
    }
}

void nRF24registers::setCE_LOW()
{
    printf("%s setCE_LOW CE=%d\n", LOGHDR,CE);
    if (CE)
    {
        CE = false;
        CEset();
    }
}

/*
void nRF24registers::printBin(byte toPrint)
{
    int i = 8;
    printf("0b");
    while (--i>=0)
    {
        printf("%d", (toPrint&0b10000000) >> 7);
        if (i==4)
        { printf(" "); }
        toPrint <<= 1;
    }
}
 */

byte* nRF24registers::read_register(byte* read_command)
{
    byte addr = *read_command&0b00011111;
    return (byte*) register_array[addr];
}

void nRF24registers::write_register(byte* bytes_to_write)
{
    byte temp;
    bool emitRXTXmodeSignal = false;
    bool emitPWRsig = false;
    byte addr = bytes_to_write[0]&0b00011111;


    //printf("%s reg addr= 0x%02x\n", LOGHDR,addr);
    //don't write to these registers
    if ((addr==eOBSERVE_TX) || (addr==eRPD) || (addr==eFIFO_STATUS))
    {
        printf("%s Register is ObserveTX eRPF or FIFOSTATUS\n", LOGHDR);
        return;
    }

    byte* where_to_write = (byte*) register_array[addr];
    if (addr==eRF_CH)
    {
        clearPLOS_CNT();
    }

    if (addr==eCONFIG)
    {
        //printf("%s old reg:0x%X " BYTE_TO_BINARY_PATTERN "\n",LOGHDR, addr,BYTE_TO_BINARY(where_to_write[0]));
        //printf("%s new reg:0x%X " BYTE_TO_BINARY_PATTERN "\n",LOGHDR, addr,BYTE_TO_BINARY(bytes_to_write[1]));
        if ((where_to_write[0]&0b1)!=(bytes_to_write[1]&0b1))
        {
            // RX/TX mode change
            emitRXTXmodeSignal = true;
        }
        if ((where_to_write[0]&0b10)!=(bytes_to_write[1]&0b10))
        {
            emitPWRsig = true;
        }
    }

    if (addr!=eSTATUS)
    {
        where_to_write[0] = bytes_to_write[1];
    }
    else
    {
        //write to STATUS.. bits are cleared by writing 1 pm the,///
        temp = bytes_to_write[1]&0b01110000; //clears bit that cant be written to
        temp = ~temp; //inverts all, 0 become 1 and  leaves those bits alone after AND op,  where 1s are bits will get cleared after AND operation
        where_to_write[0] &= temp;
    }
    //printf("byte to write[1]: 0x%02x  ", bytes_to_write[1]);
    //printf("where to write[0]: 0x%02x\n", where_to_write[0]);
    if ((addr==eRX_ADDR_P0) || (addr==eRX_ADDR_P1) || (addr==eTX_ADDR))
    {
        *((uint64_t*) where_to_write) = *((uint64_t*) (bytes_to_write + 1));
        //printf("written: %llx\n", *((uint64_t*) where_to_write));
    }

    if (emitRXTXmodeSignal==true)
    {
        //emit TXmodeSet();
        RXTXmodeSet();
    }
    if (emitPWRsig==true)
    {
        //emit PWRUPset();
        PWRset();
    }
}

uint64_t nRF24registers::getTXaddress()
{
    printf("%s TX addr=%llx\n", LOGHDR, *((uint64_t*) register_array[eTX_ADDR]));
    return *((uint64_t*) register_array[eTX_ADDR]);
}

byte nRF24registers::addressToPipe(uint64_t address)
{
    //printf("%s addr=0x%llx reg0=0x%llx\n", LOGHDR, address, *((uint64_t*) register_array[eRX_ADDR_P0]));
    //printf("%s addr=0x%llx reg1=0x%llx\n", LOGHDR, address, *((uint64_t*) register_array[eRX_ADDR_P1]));
    //printf("%s addr=0x%llx reg2=0x%x\n", LOGHDR, address, *((byte*) register_array[eRX_ADDR_P2]));
    //printf("%s addr=0x%llx reg3=0x%x\n", LOGHDR, address, *((byte*) register_array[eRX_ADDR_P3]));
    //printf("%s addr=0x%llx reg4=0x%x\n", LOGHDR, address, *((byte*) register_array[eRX_ADDR_P4]));
    //printf("%s addr=0x%llx reg5=0x%x\n", LOGHDR, address, *((byte*) register_array[eRX_ADDR_P5]));

    uint64_t msb = *((uint64_t*) register_array[eRX_ADDR_P1])&0xFFFFFFFFFFFFFF00;

    //printf("%s msb=0x%llx\n", LOGHDR, msb);
    //printf("%s addr=0x%llx msb=0x%llx\n", LOGHDR, address, msb | *((byte*) register_array[eRX_ADDR_P2]));

    if (REGISTERS.sEN_RXADDR.sERX_P0)
    {
        if (address==*((uint64_t*) register_array[eRX_ADDR_P0]))
        {
            return 0;
        }
    }

    if (REGISTERS.sEN_RXADDR.sERX_P1)
    {
        if (address==*((uint64_t*) register_array[eRX_ADDR_P1]))
        {
            return 1;
        }
    }

    if (REGISTERS.sEN_RXADDR.sERX_P2)
    {

        if (address==(msb|*((byte*) register_array[eRX_ADDR_P2])))
        {
            return 2;
        }
    }

    if (REGISTERS.sEN_RXADDR.sERX_P3)
    {
        if (address==(msb|*((byte*) register_array[eRX_ADDR_P3])))
        {
            return 3;
        }
    }

    if (REGISTERS.sEN_RXADDR.sERX_P4)
    {
        if (address==(msb|*((byte*) register_array[eRX_ADDR_P4])))
        {
            return 4;
        }
    }

    if (REGISTERS.sEN_RXADDR.sERX_P5)
    {
        if (address==(msb|*((byte*) register_array[eRX_ADDR_P5])))
        {
            return 5;
        }
    }

    return 0xFF;
}

uint8_t nRF24registers::getENAA(byte pipe)
{
    switch (pipe)
    {
        case 0:
            return REGISTERS.sEN_AA.sENAA_P0;
        case 1:
            return REGISTERS.sEN_AA.sENAA_P1;
        case 2:
            return REGISTERS.sEN_AA.sENAA_P2;
        case 3:
            return REGISTERS.sEN_AA.sENAA_P3;
        case 4:
            return REGISTERS.sEN_AA.sENAA_P4;
        case 5:
            return REGISTERS.sEN_AA.sENAA_P5;
        default:
            return 0;
    }

}

uint64_t nRF24registers::getAddressFromPipe_ENAA(byte pipe)
{
    uint64_t pipe_address = 0;
    uint8_t last_byte = 0;
    switch (pipe)
    {
        case 0:
            if (REGISTERS.sEN_RXADDR.sERX_P0 && REGISTERS.sEN_AA.sENAA_P0)
            {
                pipe_address = *((uint64_t*) register_array[eRX_ADDR_P0]);
            }
            break;
        case 1:
            if (REGISTERS.sEN_RXADDR.sERX_P1 && REGISTERS.sEN_AA.sENAA_P1)
            {
                pipe_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
            }
            break;
        case 2:
            if (REGISTERS.sEN_RXADDR.sERX_P2 && REGISTERS.sEN_AA.sENAA_P2)
            {
                pipe_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
                last_byte = *((uint8_t*) register_array[eRX_ADDR_P2]);
                pipe_address = (pipe_address&0xFFFFFFFFFFFFFF00)|(uint64_t) last_byte;
            }
            break;
        case 3:
            if (REGISTERS.sEN_RXADDR.sERX_P3 && REGISTERS.sEN_AA.sENAA_P3)
            {
                pipe_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
                last_byte = *((uint8_t*) register_array[eRX_ADDR_P3]);
                pipe_address = (pipe_address&0xFFFFFFFFFFFFFF00)|(uint64_t) last_byte;
            }
            break;
        case 4:
            if (REGISTERS.sEN_RXADDR.sERX_P4 && REGISTERS.sEN_AA.sENAA_P4)
            {
                pipe_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
                last_byte = *((uint8_t*) register_array[eRX_ADDR_P4]);
                pipe_address = (pipe_address&0xFFFFFFFFFFFFFF00)|(uint64_t) last_byte;
            }
            break;
        case 5:
            if (REGISTERS.sEN_RXADDR.sERX_P5 && REGISTERS.sEN_AA.sENAA_P5)
            {
                pipe_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
                last_byte = *((uint8_t*) register_array[eRX_ADDR_P5]);
                pipe_address = (pipe_address&0xFFFFFFFFFFFFFF00)|(uint64_t) last_byte;
            }
            break;
        default:
            return 0;
    }
    return pipe_address;
}

void nRF24registers::PLOS_CNT_INC()
{
    if (REGISTERS.sOBSERVE_TX.sPLOS_CNT<15)
    {
        REGISTERS.sOBSERVE_TX.sPLOS_CNT++;
    }
}

void nRF24registers::ARC_CNT_INC()
{
    if (REGISTERS.sOBSERVE_TX.sARC_CNT<15)
    {
        REGISTERS.sOBSERVE_TX.sARC_CNT++;
    }
}

uint64_t nRF24registers::getAddressFromPipe(byte pipe)
{
    uint64_t temp64;
    uint64_t ACK_address;
    switch (pipe)
    {
        case 0:
            if (REGISTERS.sEN_RXADDR.sERX_P0)
            {
                ACK_address = *((uint64_t*) register_array[eRX_ADDR_P0]);
            }
            else
            {
                return 0;
            }//PO RX not enabled
            break;
        case 1:
            if (REGISTERS.sEN_RXADDR.sERX_P1)
            {
                ACK_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
            }
            else
            {
                return 0;
            }//P1 RX not enabled
            break;
        case 2:
            if (REGISTERS.sEN_RXADDR.sERX_P2)
            {
                ACK_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
            }
            else
            {
                return 0;
            }//P2 RX not enabled
            temp64 = (uint64_t) (*((uint8_t*) (register_array[eRX_ADDR_P2])));
            ACK_address &= 0xFFFFFF00;
            temp64 &= 0x000000FF;
            ACK_address |= temp64;
            break;
        case 3:
            if (REGISTERS.sEN_RXADDR.sERX_P3)
            {
                ACK_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
            }
            else
            {
                return 0;
            }//P3 RX not enabled
            temp64 = (uint64_t) (*((uint8_t*) (register_array[eRX_ADDR_P3])));
            ACK_address &= 0xFFFFFF00;
            temp64 &= 0x000000FF;
            ACK_address |= temp64;
            break;
        case 4:
            if (REGISTERS.sEN_RXADDR.sERX_P4)
            {
                ACK_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
            }
            else
            {
                return 0;
            }//P4 RX not enabled
            temp64 = (uint64_t) (*((uint8_t*) (register_array[eRX_ADDR_P4])));
            ACK_address &= 0xFFFFFF00;
            temp64 &= 0x000000FF;
            ACK_address |= temp64;
            break;
        case 5:
            if (REGISTERS.sEN_RXADDR.sERX_P5)
            {
                ACK_address = *((uint64_t*) register_array[eRX_ADDR_P1]);
            }
            else
            {
                return 0;
            }//P5 RX not enabled
            temp64 = (uint64_t) (*((uint8_t*) (register_array[eRX_ADDR_P5])));
            ACK_address &= 0xFFFFFF00;
            temp64 &= 0x000000FF;
            ACK_address |= temp64;
            break;
        default:
            return 0;
    }
    return ACK_address;
}

bool nRF24registers::checkIRQ()
{
    return ((REGISTERS.sSTATUS.sRX_DR && REGISTERS.sCONFIG.sMASK_RX_DR) || (REGISTERS.sSTATUS.sTX_DS && REGISTERS.sCONFIG.sMASK_TX_DS) ||
            (REGISTERS.sSTATUS.sMAX_RT && REGISTERS.sCONFIG.sMASK_MAX_RT));
}

void nRF24registers::printRegContents()
{
    printf("---REGISTERS---\n---CONFIG---\n");
    printf("CONFIG.Reserved => %d\n", REGISTERS.sCONFIG.sReserved);
    printf("CONFIG.MASK_RX_DR => %d\n", REGISTERS.sCONFIG.sMASK_RX_DR);
    printf("CONFIG.MASK_TX_DS => %d\n", REGISTERS.sCONFIG.sMASK_TX_DS);
    printf("CONFIG.MASK_MAX_RT => %d\n", REGISTERS.sCONFIG.sMASK_MAX_RT);
    printf("CONFIG.EN_CRC => %d\n", REGISTERS.sCONFIG.sEN_CRC);
    printf("CONFIG.EN_CRC => %d\n", REGISTERS.sCONFIG.sCRC0);
    printf("CONFIG.PWR_UP => %d\n", REGISTERS.sCONFIG.sPWR_UP);
    printf("CONFIG.PRIM_RX => %d\n", REGISTERS.sCONFIG.sPRIM_RX);

    printf("\n---EN_AA---\n");
    printf("EN_AA.Reserved  => %d\n", REGISTERS.sEN_AA.sReserved);
    printf("EN_AA.ENAA_P5  => %d\n", REGISTERS.sEN_AA.sENAA_P5);
    printf("EN_AA.ENAA_P4  => %d\n", REGISTERS.sEN_AA.sENAA_P4);
    printf("EN_AA.ENAA_P3  => %d\n", REGISTERS.sEN_AA.sENAA_P3);
    printf("EN_AA.ENAA_P2  => %d\n", REGISTERS.sEN_AA.sENAA_P2);
    printf("EN_AA.ENAA_P1  => %d\n", REGISTERS.sEN_AA.sENAA_P1);
    printf("EN_AA.ENAA_P0  => %d\n", REGISTERS.sEN_AA.sENAA_P0);

    printf("\n---EN_RXADDR---\n");
    printf("EN_RXADDR.Reserved => %d\n", REGISTERS.sEN_RXADDR.sReserved);
    printf("EN_RXADDR.ERX_P5 => %d\n", REGISTERS.sEN_RXADDR.sERX_P5);
    printf("EN_RXADDR.ERX_P4 => %d\n", REGISTERS.sEN_RXADDR.sERX_P4);
    printf("EN_RXADDR.ERX_P3 => %d\n", REGISTERS.sEN_RXADDR.sERX_P3);
    printf("EN_RXADDR.ERX_P2 => %d\n", REGISTERS.sEN_RXADDR.sERX_P2);
    printf("EN_RXADDR.ERX_P1 => %d\n", REGISTERS.sEN_RXADDR.sERX_P1);
    printf("EN_RXADDR.ERX_P0 => %d\n", REGISTERS.sEN_RXADDR.sERX_P0);

    printf("\n---SETUP_AW---\n");
    printf("SETUP_AW.Reserved => %d\n", REGISTERS.sSETUP_AW.sReserved);
    printf("SETUP_AW.AW => %d\n", REGISTERS.sSETUP_AW.sAW);

    printf("\n---SETUP_RETR---\n");
    printf("SETUP_RETR.ARD => %d\n", REGISTERS.sSETUP_RETR.sARD);
    printf("SETUP_RETR.ARC => %d\n", REGISTERS.sSETUP_RETR.sARC);

    printf("\n---RF_CH---\n");
    printf("RF_CH.Reserved => %d\n", REGISTERS.sRF_CH.sReserved);
    printf("RF_CH.RF_CH => %d\n", REGISTERS.sRF_CH.sRF_CH);

    printf("\n---RF_SETUP---\n");
    printf("RF_SETUP.CONT_WAVE => %d\n", REGISTERS.sRF_SETUP.sCONT_WAVE);
    printf("RF_SETUP.Reserved => %d\n", REGISTERS.sRF_SETUP.sReserved);
    printf("RF_SETUP.RF_DR_LOW => %d\n", REGISTERS.sRF_SETUP.sRF_DR_LOW);
    printf("RF_SETUP.PLL_LOCK => %d\n", REGISTERS.sRF_SETUP.sPLL_LOCK);
    printf("RF_SETUP.RF_DR_HIGH => %d\n", REGISTERS.sRF_SETUP.sRF_DR_HIGH);
    printf("RF_SETUP.RF_PWR => %d\n", REGISTERS.sRF_SETUP.sRF_PWR);
    printf("RF_SETUP.Obsolete => %d\n", REGISTERS.sRF_SETUP.sObsolete);

    printf("\n---STATUS---\n");
    printf("STATUS.Reserved => %d\n", REGISTERS.sSTATUS.sReserved);
    printf("STATUS.RX_DR => %d\n", REGISTERS.sSTATUS.sRX_DR);
    printf("STATUS.TX_DS => %d\n", REGISTERS.sSTATUS.sTX_DS);
    printf("STATUS.MAX_RT => %d\n", REGISTERS.sSTATUS.sMAX_RT);
    printf("STATUS.RX_P_NO => %d\n", REGISTERS.sSTATUS.sRX_P_NO);
    printf("STATUS.TX_FULL => %d\n", REGISTERS.sSTATUS.sTX_FULL);

    printf("\n---OBSERVE_TX---\n");
    printf("OBSERVE_TX.PLOS_CNT => %d\n", REGISTERS.sOBSERVE_TX.sPLOS_CNT);
    printf("OBSERVE_TX.ARC_CNT => %d\n", REGISTERS.sOBSERVE_TX.sARC_CNT);

    printf("\n---RPD---\n");
    printf("RPD.Reserved => %d\n", REGISTERS.sRPD.sReserved);
    printf("RPD.RPD => %d\n", REGISTERS.sRPD.sRPD);

    printf("\n---RX_ADDR_P0---\n");
    printf("0x%llX\n", *((uint64_t*) register_array[eRX_ADDR_P0]));
    printf("\n---RX_ADDR_P1---\n");
    printf("0x%llX\n", *((uint64_t*) register_array[eRX_ADDR_P1]));
    printf("\n---RX_ADDR_P2---\n");
    printf("%X\n", *((byte*) register_array[eRX_ADDR_P2]));
    printf("\n---RX_ADDR_P3---\n");
    printf("%X\n", *((byte*) register_array[eRX_ADDR_P3]));
    printf("\n---RX_ADDR_P4---\n");
    printf("%X\n", *((byte*) register_array[eRX_ADDR_P4]));
    printf("\n---RX_ADDR_P5---\n");
    printf("%X\n", *((byte*) register_array[eRX_ADDR_P5]));
    printf("\n---TX_ADDR---\n");
    printf("0x%llX\n", *((uint64_t*) register_array[eTX_ADDR]));

    printf("\n---RX_PW_P0---\n");
    printf("RX_PW_P0.Reserved => %d\n", REGISTERS.sRX_PW_P0.sReserved);
    printf("PX_PW_P0.RX_PW_P0 => %d\n", REGISTERS.sRX_PW_P0.sRX_PW_P);
    printf("\n---RX_PW_P1---\n");
    printf("RX_PW_P1.Reserved => %d\n", REGISTERS.sRX_PW_P1.sReserved);
    printf("PX_PW_P1.RX_PW_P => %d\n", REGISTERS.sRX_PW_P1.sRX_PW_P);
    printf("\n---RX_PW_P2---\n");
    printf("RX_PW_P2.Reserved => %d\n", REGISTERS.sRX_PW_P2.sReserved);
    printf("PX_PW_P2.RX_PW_P => %d\n", REGISTERS.sRX_PW_P2.sRX_PW_P);
    printf("\n---RX_PW_P3---\n");
    printf("RX_PW_P3.Reserved => %d\n", REGISTERS.sRX_PW_P3.sReserved);
    printf("PX_PW_P3.RX_PW_P => %d\n", REGISTERS.sRX_PW_P3.sRX_PW_P);
    printf("\n---RX_PW_P4---\n");
    printf("RX_PW_P4.Reserved => %d\n", REGISTERS.sRX_PW_P4.sReserved);
    printf("PX_PW_P4.RX_PW_P => %d\n", REGISTERS.sRX_PW_P4.sRX_PW_P);
    printf("\n---RX_PW_P5---\n");
    printf("RX_PW_P5.Reserved => %d\n", REGISTERS.sRX_PW_P5.sReserved);
    printf("PX_PW_P5.RX_PW_P => %d\n", REGISTERS.sRX_PW_P5.sRX_PW_P);

    printf("\n---FIFO_STATUS---\n");
    printf("FIFO_STATUS.Reserved2 => %d\n", REGISTERS.sFIFO_STATUS.sReserved2);
    printf("FIFO_STATUS.TX_REUSE => %d\n", REGISTERS.sFIFO_STATUS.sTX_REUSE);
    printf("FIFO_STATUS.TX_FULL => %d\n", REGISTERS.sFIFO_STATUS.sTX_FULL);
    printf("FIFO_STATUS.TX_EMPTY => %d\n", REGISTERS.sFIFO_STATUS.sTX_EMPTY);
    printf("FIFO_STATUS.Reserved1 => %d\n", REGISTERS.sFIFO_STATUS.sReserved1);
    printf("FIFO_STATUS.RX_FULL => %d\n", REGISTERS.sFIFO_STATUS.sRX_FULL);
    printf("FIFO_STATUS.RX_EMPTY => %d\n", REGISTERS.sFIFO_STATUS.sRX_EMPTY);

    printf("\n---DYNPD---\n");
    printf("DYNPD.Reserved => %d\n", REGISTERS.sDYNPD.sReserved);
    printf("DYNPD.DPL_P5 => %d\n", REGISTERS.sDYNPD.sDPL_P5);
    printf("DYNPD.DPL_P4 => %d\n", REGISTERS.sDYNPD.sDPL_P4);
    printf("DYNPD.DPL_P3 => %d\n", REGISTERS.sDYNPD.sDPL_P3);
    printf("DYNPD.DPL_P2 => %d\n", REGISTERS.sDYNPD.sDPL_P2);
    printf("DYNPD.DPL_P1 => %d\n", REGISTERS.sDYNPD.sDPL_P1);
    printf("DYNPD.DPL_P0 => %d\n", REGISTERS.sDYNPD.sDPL_P0);

    printf("\n---FEATURE---\n");
    printf("FEATURE.Reserved => %d\n", REGISTERS.sFEATURE.sReserved);
    printf("FEATURE.EN_DPL => %d\n", REGISTERS.sFEATURE.sEN_DPL);
    printf("FEATURE.EN_ACK_PAY => %d\n", REGISTERS.sFEATURE.sEN_ACK_PAY);
    printf("FEATURE.EN_DYN_ACK => %d\n", REGISTERS.sFEATURE.sEN_DYN_ACK);
}
