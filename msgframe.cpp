//
// Created by Hopper II, Alan T on 3/27/18.
//
#include "msgframe.h"

std::string sFRAME::toString()
{
    std::stringstream stream;
    stream << std::hex << "addr=0x" << Address << " len=" << std::dec << +Packet_Control_Field.Payload_length;
    stream << " PID=" << std::bitset<2>(Packet_Control_Field.PID);
    stream << " NP_ACK=" << std::bitset<1>(Packet_Control_Field.NP_ACK);
    std::string result( stream.str() );

    return result;
}
