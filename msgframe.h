#ifndef MSGFRAME_H
#define MSGFRAME_H
#include <cstdint>
#include <sstream>
#include <string>
#include <bitset>

typedef struct sPACKET_CONTROL_FIELD
{
    uint8_t Payload_length:6;
    uint8_t PID:2;
    uint8_t NO_ACK:1;
}tPACKET_CONTROL_FIELD;

typedef struct sFRAME
{

    //This is used to prevent "hearing" our own transmissions
    //due to Ether simulation
    int radioId = -1;
    uint64_t Address;
    tPACKET_CONTROL_FIELD Packet_Control_Field;
    uint8_t Payload[32];
    uint8_t crc[2];

    std::string toString();



}tMsgFrame;


#endif // MSGFRAME_H
