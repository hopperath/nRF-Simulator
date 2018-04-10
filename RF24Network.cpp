/*
 Copyright (C) 2011 James Coliz, Jr. <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include "RF24Network_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include "RF24.h"
#include "RF24Network.h"

extern unsigned int millis();

#if defined (ENABLE_SLEEP_MODE) && !defined (RF24_LINUX) && !defined (__ARDUINO_X86__)
#include <avr/sleep.h>
#include <avr/power.h>
volatile byte sleep_cycles_remaining;
volatile bool wasInterrupted;
#endif

uint16_t RF24NetworkHeader::next_id = 1;

#if defined ENABLE_NETWORK_STATS
uint32_t RF24Network::nFails = 0;
uint32_t RF24Network::nOK = 0;
#endif

uint32_t RF24Network::millis()
{
    return mcuClock.millis();
}

/******************************************************************/
RF24Network::RF24Network(RF24& _radio, uint16_t _txTimeout, MCUClock& _mcuclock) : radio(_radio), frame_size(MAX_FRAME_SIZE),
                                                                                   mcuClock(_mcuclock)
{
    txTimeout = _txTimeout;
    txTime = 0;
    networkFlags = 0;
    returnSysMsgs = false;
    multicastRelay = false;
}

/******************************************************************/

void RF24Network::begin(uint16_t _node_address)
{
    begin(USE_CURRENT_CHANNEL, _node_address, txTimeout);
}

void RF24Network::begin(uint16_t _node_address, uint16_t _txTimeout)
{
    begin(USE_CURRENT_CHANNEL, _node_address, _txTimeout);
}

/******************************************************************/
void RF24Network::begin(uint8_t _channel, uint16_t _node_address, uint16_t _txTimeout)
{
    printf("%s begin NETWORK V2.0.0\n", radio.rf24->LOGHDR);

    if (!is_valid_address(_node_address))
    {
        return;
    }

    node_address = _node_address;

    // Set up the radio the way we want it to look
    if (_channel!=USE_CURRENT_CHANNEL)
    {
        radio.setChannel(_channel);
    }
    //radio.enableDynamicAck();

    radio.setAutoAck(0, 0);
    radio.enableDynamicPayloads();

    // Use different retry periods to reduce data collisions
    uint8_t retryVar = (((node_address % 6) + 1) * 2) + 3;
    radio.setRetries(retryVar, 5); // max about 85ms per attempt
    txTimeout = _txTimeout;
    routeTimeout = addressToLevel(node_address) * txTimeout; // Adjust for max delay per node within a single chain

    printf("%s: txTimeout=%u routeTimeout=%u\n", radio.rf24->LOGHDR, txTimeout, routeTimeout);

    // Setup our address helper cache
    setup_address();

    // Open up all listening pipes
    uint8_t i = 6;
    while (i--)
    {
        radio.openReadingPipe(i, pipe_address(_node_address, i));
    }
    radio.startListening();
}

uint8_t RF24Network::addressToLevel(uint16_t node_addr)
{
    uint16_t node_mask_check = 0xFFFF;
    uint8_t level = 0;

    while (node_addr&node_mask_check)
    {
        node_mask_check <<= 3;
        level++;
    }

    return level;
}

uint16_t RF24Network::getAddress()
{
    return node_address;
}

/******************************************************************/

#if defined ENABLE_NETWORK_STATS
void RF24Network::failures(uint32_t *_fails, uint32_t *_ok){
    *_fails = nFails;
    *_ok = nOK;
}
#endif

/******************************************************************/

uint8_t RF24Network::update(void)
{
    // if there is data ready
    uint8_t pipe_num;
    uint8_t returnVal = 0;

    // If bypass is enabled, continue although incoming user data may be dropped
    // Allows system payloads to be read while user cache is full
    // Incoming Hold prevents data from being read from the radio, preventing incoming payloads from being acked

#if !defined (RF24_LINUX)
    if (!(networkFlags&FLAG_BYPASS_HOLDS))
    {
        if ((networkFlags&FLAG_HOLD_INCOMING) || (next_frame - frame_queue) + 34>MAIN_BUFFER_SIZE)
        {
            if (!available())
            {
                networkFlags &= ~FLAG_HOLD_INCOMING;
            }
            else
            {
                return 0;
            }
        }
    }
#endif

    //printf("%s RF24Network::update\n",radio.rf24->LOGHDR);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    while (radio.available(&pipe_num))
    {

        if ((frame_size = radio.getDynamicPayloadSize())<sizeof(RF24NetworkHeader))
        {
            delay(10);
            continue;
        }

        // Dump the payloads until we've gotten everything
        // Fetch the payload, and see if this was the last one.
        radio.read(frame_buffer, frame_size);

        // Read the beginning of the frame as the header
        RF24NetworkHeader* header = (RF24NetworkHeader*) (&frame_buffer);

        IF_RF24_SERIAL_DEBUG(printf_P("%s %u: MAC Received on %u %s\n", radio.rf24->LOGHDR, millis(), pipe_num, header->toString()));

        // Throw it away if it's not a valid address
        if (!is_valid_address(header->to_node))
        {
            continue;
        }

        uint8_t returnVal = header->type;

        // Is this for us?
        if (header->to_node==node_address)
        {

            if (header->type==NETWORK_PING)
            {
                continue;
            }
            else if (header->type==NETWORK_ADDR_RESPONSE)
            {
                uint16_t requester = MESH_DEFAULT_ADDRESS;
                if (requester!=node_address)
                {
                    header->to_node = requester;
                    int cnt = 0;
                    while (cnt++<2 && write(header->to_node, USER_TX_TO_PHYSICAL_ADDRESS))
                    {
                        delay(10);
                    }
#if defined (SERIAL_DEBUG_MINIMAL)
                    printf("Fwd add response to 0%o\n",requester);
#endif
                    continue;
                }
            }
            else if (header->type==NETWORK_REQ_ADDRESS && node_address)
            {
                printf("%s Fwd addr req to 0\n",radio.rf24->LOGHDR);
                header->from_node = node_address;
                header->to_node = MASTER_NODE;
                write(header->to_node, TX_NORMAL);
                continue;
            }

            if ((returnSysMsgs && header->type>127) || header->type==NETWORK_ACK)
            {
                IF_RF24_SERIAL_DEBUG_ROUTING(printf_P(PSTR("%s %lu MAC: System payload rcvd %d\n"), radio.rf24->LOGHDR, millis(), returnVal););
                //if( (header->type < 148 || header->type > 150) && header->type != NETWORK_MORE_FRAGMENTS_NACK && header->type != EXTERNAL_DATA_TYPE && header->type!= NETWORK_LAST_FRAGMENT){
                if (header->type!=NETWORK_FIRST_FRAGMENT && header->type!=NETWORK_MORE_FRAGMENTS &&
                    header->type!=NETWORK_MORE_FRAGMENTS_NACK && header->type!=EXTERNAL_DATA_TYPE && header->type!=NETWORK_LAST_FRAGMENT)
                {
                    return returnVal;
                }
            }

            if (enqueue(header)==2)
            { //External data received
#if defined (SERIAL_DEBUG_MINIMAL)
                printf("ret ext\n");
#endif
                return EXTERNAL_DATA_TYPE;
            }
        }
        else
        {
            //Don't forward addressed multicast packets
            if (pipe_num==MULTICAST_PIPE)
            {
                if (header->to_node==MULTICAST_ADDRESS_NODE)
                {
                    if (header->type==NETWORK_POLL)
                    {
                        //Don't respond if FLAG_NO_POLL set or this node has not joined yet.
                        // i.e. node_address = MESH_DEFAULT_ADDRESS
                        if (!(networkFlags&FLAG_NO_POLL) && node_address!=MESH_DEFAULT_ADDRESS)
                        {
                            printf("%s responding to 194 from %o\n", radio.rf24->LOGHDR, header->from_node);
                            header->to_node = header->from_node;
                            header->from_node = node_address;
                            delay(parent_pipe);
                            write(header->to_node, USER_TX_TO_PHYSICAL_ADDRESS);
                        }
                        continue;
                    }

                    enqueue(header);

                    if (multicastRelay)
                    {
                        IF_RF24_SERIAL_DEBUG_ROUTING(
                                printf_P(PSTR("%s %u MAC: FWD multicast frame from 0%o to level %u\n"), radio.rf24->LOGHDR, millis(),
                                         header->from_node, multicast_level + 1););
                        write(levelToAddress(multicast_level) << 3, 4);
                    }
                }
                else
                {
                    printf("%s multicast pkt ignored\n", radio.rf24->LOGHDR);
                }

            }
            else
            {
                write(header->to_node, 1);    //Send it on, indicate it is a routed payload
            }
        }

    }
    return returnVal;
}

#if defined (RF24_LINUX)
/******************************************************************/

uint8_t RF24Network::enqueue(RF24NetworkHeader* header)
{

    RF24NetworkFrame frame = RF24NetworkFrame(*header, frame_buffer + sizeof(RF24NetworkHeader), frame_size - sizeof(RF24NetworkHeader));

    IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%s %u: NET Enqueue @%lx "), radio.rf24->LOGHDR, millis(), frame_queue.size()));
    // Copy the current frame into the frame queue
    frame_queue.push(frame);

    return true;
}

#else // Not defined RF24_Linux:

/******************************************************************/
/******************************************************************/

uint8_t RF24Network::enqueue(RF24NetworkHeader* header)
{
    bool result = false;
    uint16_t message_size = frame_size - sizeof(RF24NetworkHeader);

    IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%lu: NET Enqueue @%x "), millis(), next_frame - frame_queue));


    // Copy the current frame into the frame queue

    if (message_size + (next_frame - frame_queue)<=MAIN_BUFFER_SIZE)
    {
        memcpy(next_frame, &frame_buffer, 8);
        memcpy(next_frame + 8, &message_size, 2);
        memcpy(next_frame + 10, frame_buffer + 8, message_size);

        //IF_RF24_SERIAL_DEBUG_FRAGMENTATION( for(int i=0; i<message_size;i++){ Serial.print(next_frame[i],HEX); Serial.print(" : "); } Serial.println(""); );

        next_frame += (message_size + 10);
#if !defined(ARDUINO_ARCH_AVR)
        if (uint8_t padding = (message_size + 10) % 4)
        {
            next_frame += 4 - padding;
        }
#endif
        //IF_RF24_SERIAL_DEBUG_FRAGMENTATION( Serial.print("Enq "); Serial.println(next_frame-frame_queue); );//printf_P(PSTR("enq %d\n"),next_frame-frame_queue); );

        result = true;
    }
    else
    {
        result = false;
        IF_RF24_SERIAL_DEBUG(printf_P(PSTR("NET **Drop Payload** Buffer Full")));
    }
    return result;
}

#endif //End not defined RF24_Linux

/******************************************************************/

bool RF24Network::available(void)
{
    return (!frame_queue.empty());
}

/******************************************************************/

uint16_t RF24Network::parent() const
{
    if (node_address==0)
    {
        return -1;
    }
    else
    {
        return parent_node;
    }
}

/******************************************************************/
/*uint8_t RF24Network::peekData(){

        return frame_queue[0];
}*/

uint16_t RF24Network::peek(RF24NetworkHeader& header)
{
    if (available())
    {
#if defined (RF24_LINUX)
        RF24NetworkFrame frame = frame_queue.front();
        memcpy(&header, &frame.header, sizeof(RF24NetworkHeader));
        return frame.message_size;
#else
        RF24NetworkFrame* frame = (RF24NetworkFrame*) (frame_queue);
        memcpy(&header, &frame->header, sizeof(RF24NetworkHeader));
        uint16_t msg_size;
        memcpy(&msg_size, frame_queue + 8, 2);
        return msg_size;
#endif
    }
    return 0;
}

/******************************************************************/

uint16_t RF24Network::read(RF24NetworkHeader& header, void* message, uint16_t maxlen)
{
    uint16_t bufsize = 0;

#if defined (RF24_LINUX)
    if (available())
    {
        RF24NetworkFrame frame = frame_queue.front();

        // How much buffer size should we actually copy?
        bufsize = rf24_min(frame.message_size, maxlen);
        memcpy(&header, &(frame.header), sizeof(RF24NetworkHeader));
        memcpy(message, frame.message_buffer, bufsize);

        IF_RF24_SERIAL_DEBUG(printf("%s %u: FRG message size %i\n", radio.rf24->LOGHDR, millis(), frame.message_size););
        IF_RF24_SERIAL_DEBUG(printf("%s %u: FRG message ", radio.rf24->LOGHDR, millis());
                                     const char* charPtr = reinterpret_cast<const char*>(message);
                                     for (uint16_t i = 0; i<bufsize; i++)
                                     {
                                         printf("%02X ", charPtr[i]);
                                     };
                                     printf("\n"));

        IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%s %u: NET read %s\n"), radio.rf24->LOGHDR, millis(), header.toString()));

        frame_queue.pop();
    }
#else
    if (available())
    {

        memcpy(&header, frame_queue, 8);
        memcpy(&bufsize, frame_queue + 8, 2);

        if (maxlen>0)
        {
            maxlen = rf24_min(maxlen, bufsize);
            memcpy(message, frame_queue + 10, maxlen);
            IF_RF24_SERIAL_DEBUG(printf("%lu: NET message size %d\n", millis(), bufsize););

            IF_RF24_SERIAL_DEBUG(uint16_t
            len = maxlen;
            printf_P(PSTR("%lu: NET r message "), millis());
            const uint8_t* charPtr = reinterpret_cast<const uint8_t*>(message);
            while (len--)
            { printf("%02x ", charPtr[len]); }
            printf_P(PSTR("\n")));

        }
        next_frame -= bufsize + 10;
        uint8_t padding = 0;
#if !defined(ARDUINO_ARCH_AVR)
        if ((padding = (bufsize + 10) % 4))
        {
            padding = 4 - padding;
            next_frame -= padding;
        }
#endif
        memmove(frame_queue, frame_queue + bufsize + 10 + padding, sizeof(frame_queue) - bufsize);
        //IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%lu: NET Received %s\n"),millis(),header.toString()));
    }
#endif
    return bufsize;
}

/******************************************************************/
bool RF24Network::multicast(RF24NetworkHeader& header, const void* message, uint16_t len, uint8_t level)
{
    // Fill out the header
    header.to_node = MULTICAST_ADDRESS_NODE;
    header.from_node = node_address;
    return write(header, message, len, levelToAddress(level));
}

/******************************************************************/
bool RF24Network::write(RF24NetworkHeader& header, const void* message, uint16_t len)
{
    return write(header, message, len, 070);
}
/******************************************************************/
bool RF24Network::write(RF24NetworkHeader& header, const void* message, uint16_t len, uint16_t writeDirect)
{

    //Allows time for requests (RF24Mesh) to get through between failed writes on busy nodes
    while (millis() - txTime<25)
    {
        YIELD();
        if (update()>127)
        { break; }
    }
    delayMicroseconds(200);

    frame_size = rf24_min(len + sizeof(RF24NetworkHeader), MAX_FRAME_SIZE);
    return _write(header, message, rf24_min(len, max_frame_payload_size), writeDirect);
}
/******************************************************************/

bool RF24Network::_write(RF24NetworkHeader& header, const void* message, uint16_t len, uint16_t writeDirect)
{
    // Fill out the header
    header.from_node = node_address;

    // Build the full frame to send
    memcpy(frame_buffer, &header, sizeof(RF24NetworkHeader));

    IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%s %u: NET Sending %s\n"), radio.rf24->LOGHDR, millis(), header.toString()));

    if (len)
    {
#if defined (RF24_LINUX)
        memcpy(frame_buffer + sizeof(RF24NetworkHeader), message, rf24_min(frame_size - sizeof(RF24NetworkHeader), len));
        IF_RF24_SERIAL_DEBUG(printf("%s %u: FRG frame size %i\n", radio.rf24->LOGHDR, millis(), frame_size););
        IF_RF24_SERIAL_DEBUG(printf("%s %u: FRG frame ", radio.rf24->LOGHDR, millis());
                                     const char* charPtr = reinterpret_cast<const char*>(frame_buffer);
                                     for (uint16_t i = 0; i<frame_size; i++)
                                     { printf("%02X ", charPtr[i]); };
                                     printf("\n"));
#else

        memcpy(frame_buffer + sizeof(RF24NetworkHeader), message, len);

        IF_RF24_SERIAL_DEBUG(uint16_t
        tmpLen = len;
        printf_P(PSTR("%lu: NET message "), millis());
        const uint8_t* charPtr = reinterpret_cast<const uint8_t*>(message);
        while (tmpLen--)
        { printf("%02x ", charPtr[tmpLen]); }
        printf_P(PSTR("\n")));
#endif
    }

    if (writeDirect!=070)
    {
        uint8_t sendType = USER_TX_TO_LOGICAL_ADDRESS; // Payload is multicast to the first node, and routed normally to the next

        if (header.to_node==MULTICAST_ADDRESS_NODE)
        {
            sendType = USER_TX_MULTICAST;
        }
        if (header.to_node==writeDirect)
        {
            sendType = USER_TX_TO_PHYSICAL_ADDRESS; // Payload is multicast to the first node, which is the recipient
        }
        return write(writeDirect, sendType);
    }
    return write(header.to_node, TX_NORMAL);
}

/******************************************************************/

bool RF24Network::write(uint16_t to_node,
                        uint8_t directTo)  // Direct To: 0 = First Payload, standard routing, 1=routed payload, 2=directRoute to host, 3=directRoute to Route
{
    IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%s RF24Network::write to_node=0%o directTo=%d\n"),radio.rf24->LOGHDR,to_node,directTo));
    bool ok = false;
    bool isAckType = false;
    if (frame_buffer[6]>64 && frame_buffer[6]<192)
    {
        isAckType = true;
    }

    // Throw it away if it's not a valid address
    if (!is_valid_address(to_node))
    {
        return false;
    }

    //Load info into our conversion structure, and get the converted address info
    logicalToPhysicalStruct conversion = {to_node, directTo, 0};
    logicalToPhysicalAddress(&conversion);

#if defined (RF24_LINUX)
    IF_RF24_SERIAL_DEBUG( printf_P(PSTR("%s %u: MAC Sending to 0%o via 0%o on pipe %x type=%d\n"), radio.rf24->LOGHDR, millis(), to_node, conversion.send_node,conversion.send_pipe,frame_buffer[6]));
#else
    IF_RF24_SERIAL_DEBUG(
            printf_P(PSTR("%lu: MAC Sending to 0%o via 0%o on pipe %x\n"), millis(), to_node, conversion.send_node, conversion.send_pipe));
#endif
    /**Write it*/
    ok = write_to_pipe(conversion.send_node, conversion.send_pipe, conversion.multicast);

    if (!ok)
    {
#if defined (RF24_LINUX)
        IF_RF24_SERIAL_DEBUG_ROUTING(
                printf_P(PSTR("%s %u: MAC Send fail to 0%o via 0%o on pipe %x\n"), radio.rf24->LOGHDR, millis(), to_node, conversion.send_node,
                         conversion.send_pipe));
    }
#else
    IF_RF24_SERIAL_DEBUG_ROUTING(
            printf_P(PSTR("%lu: MAC Send fail to 0%o via 0%o on pipe %x\n"), millis(), to_node, conversion.send_node,
                     conversion.send_pipe););
}
#endif

    if (directTo==TX_ROUTED && ok && conversion.send_node==to_node && isAckType)
    {

        auto header = (RF24NetworkHeader*) &frame_buffer;
        header->type = NETWORK_ACK;                    // Set the payload type to NETWORK_ACK
        header->to_node = header->from_node;          // Change the 'to' address to the 'from' address

        conversion.send_node = header->from_node;
        conversion.send_pipe = TX_ROUTED;
        conversion.multicast = false;
        logicalToPhysicalAddress(&conversion);

        //Write the data using the resulting physical address
        frame_size = sizeof(RF24NetworkHeader);
        write_to_pipe(conversion.send_node, conversion.send_pipe, conversion.multicast);

        //dynLen=0;
#if defined (RF24_LINUX)
        IF_RF24_SERIAL_DEBUG_ROUTING(
                printf_P(PSTR("%s %u MAC: Route OK to 0%o ACK sent to 0%o\n"), radio.rf24->LOGHDR, millis(), to_node, header->from_node););
#else
        IF_RF24_SERIAL_DEBUG_ROUTING(printf_P(PSTR("%lu MAC: Route OK to 0%o ACK sent to 0%o\n"), millis(), to_node, header->from_node););
#endif
    }

    if (ok && conversion.send_node!=to_node &&
        (directTo==TX_NORMAL || directTo==USER_TX_TO_LOGICAL_ADDRESS) && isAckType)
    {
        // Now, continue listening
        if (networkFlags&FLAG_FAST_FRAG)
        {
            radio.txStandBy(txTimeout);
            networkFlags &= ~FLAG_FAST_FRAG;
            radio.setAutoAck(0, 0);
        }
        radio.startListening();

        uint32_t reply_time = millis();

        //TODO: SIM ONLY
        int yield = 0;
        while (update()!=NETWORK_ACK)
        {
#if defined (RF24_LINUX)
            delayMicroseconds(900);
#endif
            if (millis() - reply_time>routeTimeout)
            {
#if defined (RF24_LINUX)
                IF_RF24_SERIAL_DEBUG_ROUTING(
                        printf_P(PSTR("%s %u: MAC Network ACK fail from 0%o via 0%o on pipe %x\n"), radio.rf24->LOGHDR, millis(), to_node,
                                 conversion.send_node, conversion.send_pipe););
#else
                IF_RF24_SERIAL_DEBUG_ROUTING(
                        printf_P(PSTR("%lu: MAC Network ACK fail from 0%o via 0%o on pipe %x\n"), millis(), to_node, conversion.send_node,
                                 conversion.send_pipe););
#endif
                ok = false;
                break;
            }
            YIELDAT(5);
        }
    }

    printf("%s networkFlags: " BYTE_TO_BINARY_PATTERN "\n",radio.rf24->LOGHDR,BYTE_TO_BINARY(networkFlags));

    if (!(networkFlags&FLAG_FAST_FRAG))
    {
        // Now, continue listening
        printf("%s write:startListening\n",radio.rf24->LOGHDR);

        radio.startListening();
    }

#if defined ENABLE_NETWORK_STATS
        if(ok == true){
                  ++nOK;
        }else{	++nFails;
        }
#endif
    IF_RF24_SERIAL_DEBUG_ROUTING(printf_P(PSTR("%s %u: MAC Send ok=%x\n"), radio.rf24->LOGHDR, ok););
    return ok;
}

/******************************************************************/

// Provided the to_node and directTo option, it will return the resulting node and pipe
bool RF24Network::logicalToPhysicalAddress(logicalToPhysicalStruct* conversionInfo)
{

    //Create pointers so this makes sense.. kind of
    //We take in the to_node(logical) now, at the end of the function, output the send_node(physical) address, etc.
    //back to the original memory address that held the logical information.
    uint16_t* to_node = &conversionInfo->send_node;
    uint8_t* directTo = &conversionInfo->send_pipe;
    bool* multicast = &conversionInfo->multicast;

    // Where do we send this?  By default, to our parent
    uint16_t pre_conversion_send_node = parent_node;

    // On which pipe
    uint8_t pre_conversion_send_pipe = parent_pipe;

    if (*directTo>TX_ROUTED)
    {
        pre_conversion_send_node = *to_node;
        *multicast = 1;
        //if(*directTo == USER_TX_MULTICAST || *directTo == USER_TX_TO_PHYSICAL_ADDRESS){
        pre_conversion_send_pipe = 0;
        //}
    }
        // If the node is a direct child,
    else if (is_direct_child(*to_node))
    {
        // Send directly
        pre_conversion_send_node = *to_node;
        // To its listening pipe
        pre_conversion_send_pipe = 5;
    }
        // If the node is a child of a child
        // talk on our child's listening pipe,
        // and let the direct child relay it.
    else if (is_descendant(*to_node))
    {
        pre_conversion_send_node = direct_child_route_to(*to_node);
        pre_conversion_send_pipe = 5;
    }

    *to_node = pre_conversion_send_node;
    *directTo = pre_conversion_send_pipe;

    return 1;

}

/********************************************************/


bool RF24Network::write_to_pipe(uint16_t node, uint8_t pipe, bool multicast)
{
    bool ok = false;
    uint64_t out_pipe = pipe_address(node, pipe);

    // Open the correct pipe for writing.
    // First, stop listening so we can talk

    if (!(networkFlags&FLAG_FAST_FRAG))
    {
        radio.stopListening();
    }

    if (multicast)
    {
        radio.setAutoAck(0, 0);
    }
    else
    {
        radio.setAutoAck(0, 1);
    }

    radio.openWritingPipe(out_pipe);

    ok = radio.writeFast(frame_buffer, frame_size, 0);

    if (!(networkFlags&FLAG_FAST_FRAG))
    {
        ok = radio.txStandBy(txTimeout);
        radio.setAutoAck(0, 0);
    }
    else
    {
        printf("%s write_to_pipe before waitForTX timeout=%u\n",radio.rf24->LOGHDR,100);
        radio.rf24->waitForTX(100);
    }

#if defined (__arm__) || defined (RF24_LINUX)
    IF_RF24_SERIAL_DEBUG(
            printf_P(PSTR("%s %u: MAC Sent on %x %s\n"), radio.rf24->LOGHDR, millis(), (uint32_t) out_pipe, ok ? PSTR("ok") : PSTR("failed")));
#else
    IF_RF24_SERIAL_DEBUG(printf_P(PSTR("%lu: MAC Sent on %lx %S\n"), millis(), (uint32_t) out_pipe, ok ? PSTR("ok") : PSTR("failed")));
#endif
    return ok;
}

/******************************************************************/

const char* RF24NetworkHeader::toString(void) const
{
    static char buffer[45];
    //snprintf_P(buffer,sizeof(buffer),PSTR("id %04x from 0%o to 0%o type %c"),id,from_node,to_node,type);
    sprintf_P(buffer, PSTR("%u from 0%o to 0%o type %d"), id, from_node, to_node, type);
    return buffer;
}

/******************************************************************/

bool RF24Network::is_direct_child(uint16_t node)
{
    bool result = false;

    // A direct child of ours has the same low numbers as us, and only
    // one higher number.
    //
    // e.g. node 0234 is a direct child of 034, and node 01234 is a
    // descendant but not a direct child

    // First, is it even a descendant?
    if (is_descendant(node))
    {
        // Does it only have ONE more level than us?
        uint16_t child_node_mask = (~node_mask) << 3;
        result = (node&child_node_mask)==0;
    }
    return result;
}

/******************************************************************/

bool RF24Network::is_descendant(uint16_t node)
{
    return (node&node_mask)==node_address;
}

/******************************************************************/

void RF24Network::setup_address(void)
{
    // First, establish the node_mask
    uint16_t node_mask_check = 0xFFFF;
    uint8_t count = 0;

    while (node_address&node_mask_check)
    {
        node_mask_check <<= 3;
        count++;
    }

    multicast_level = count;

    node_mask = ~node_mask_check;

    // parent mask is the next level down
    uint16_t parent_mask = node_mask >> 3;

    // parent node is the part IN the mask
    parent_node = node_address&parent_mask;

    // parent pipe is the part OUT of the mask
    uint16_t i = node_address;
    uint16_t m = parent_mask;
    while (m)
    {
        i >>= 3;
        m >>= 3;
    }
    parent_pipe = i;

    IF_RF24_SERIAL_DEBUG_MINIMAL(
            printf_P(PSTR("%s setup_address node=0%o mask=0%o parent=0%o pipe=0%o\n"), radio.rf24->LOGHDR, node_address, node_mask,
                     parent_node, parent_pipe););

}

/******************************************************************/
uint16_t RF24Network::addressOfPipe(uint16_t node, uint8_t pipeNo)
{
    //Say this node is 013 (1011), mask is 077 or (00111111)
    //Say we want to use pipe 3 (11)
    //6 bits in node mask, so shift pipeNo 6 times left and | into address
    uint16_t m = node_mask >> 3;
    uint8_t i = 0;

    while (m)
    {       //While there are bits left in the node mask
        m >>= 1;     //Shift to the right
        i++;       //Count the # of increments
    }
    return node|(pipeNo << i);
}

/******************************************************************/

uint16_t RF24Network::direct_child_route_to(uint16_t node)
{
    // Presumes that this is in fact a child!!
    uint16_t child_mask = (node_mask << 3)|0x07;
    return node&child_mask;

}

/******************************************************************/
/*
uint8_t RF24Network::pipe_to_descendant( uint16_t node )
{
  uint16_t i = node;
  uint16_t m = node_mask;

  while (m)
  {
    i >>= 3;
    m >>= 3;
  }

  return i & 0B111;
}*/

/******************************************************************/

bool RF24Network::is_valid_address(uint16_t node)
{
    bool result = true;

    while (node)
    {
        uint8_t digit = node&0x07;
        if (digit<0 || digit>5)    //Allow our out of range multicast address
        {
            result = false;
            IF_RF24_SERIAL_DEBUG_MINIMAL(printf_P(PSTR("%s *** WARNING *** Invalid address 0%o\n"), radio.rf24->LOGHDR, node););
            break;
        }
        node >>= 3;
    }

    return result;
}

/******************************************************************/
void RF24Network::multicastLevel(uint8_t level)
{
    multicast_level = level;
    //radio.stopListening();
    radio.openReadingPipe(0, pipe_address(levelToAddress(level), 0));
    //radio.startListening();
}

uint16_t RF24Network::levelToAddress(uint8_t level)
{

    uint16_t levelAddr = 1;
    if (level)
    {
        levelAddr = levelAddr << ((level - 1) * 3);
    }
    else
    {
        return 0;
    }
    return levelAddr;
}
/******************************************************************/

uint64_t RF24Network::pipe_address(uint16_t node, uint8_t pipe)
{

    static uint8_t address_translation[] = {0xc3, 0x3c, 0x33, 0xce, 0x3e, 0xe3, 0xec};
    uint64_t result = 0xCCCCCCCCCCLL;
    auto out = reinterpret_cast<uint8_t*>(&result);

    // Translate the address to use our optimally chosen radio address bytes
    uint8_t count = 1;
    uint16_t dec = node;

    while (dec)
    {
        if (pipe!=0 || !node)
        {
            out[count] = address_translation[(dec % 8)];
        }        // Convert our decimal values to octal, translate them to address bytes, and set our address

        dec /= 8;
        count++;
    }

    if (pipe!=0 || !node)
    {
        out[0] = address_translation[pipe];
    }
    else
    {
        out[1] = address_translation[count - 1];
    }

#if defined (RF24_LINUX)
    IF_RF24_SERIAL_DEBUG(uint32_t* top = reinterpret_cast<uint32_t*>(out + 1);
                                 printf_P(PSTR("%s %u: NET Pipe %i on node 0%o has address %x%x\n"), radio.rf24->LOGHDR, millis(), pipe, node,
                                          *top, *out));
#else
    IF_RF24_SERIAL_DEBUG(uint32_t * top = reinterpret_cast<uint32_t*>(out + 1);
    printf_P(PSTR("%lu: NET Pipe %i on node 0%o has address %lx%x\n"), millis(), pipe, node, *top, *out));
#endif

    return result;
}


/************************ Sleep Mode ******************************************/


#if defined ENABLE_SLEEP_MODE

#if !defined(__arm__) && !defined(__ARDUINO_X86__)

void wakeUp(){
  wasInterrupted=true;
  sleep_cycles_remaining = 0;
}

ISR(WDT_vect){
  --sleep_cycles_remaining;
}


bool RF24Network::sleepNode( unsigned int cycles, int interruptPin, uint8_t INTERRUPT_MODE){
  sleep_cycles_remaining = cycles;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  if(interruptPin != 255){
    wasInterrupted = false; //Reset Flag
    //LOW,CHANGE, FALLING, RISING correspond with the values 0,1,2,3 respectively
    attachInterrupt(interruptPin,wakeUp, INTERRUPT_MODE);
      //if(INTERRUPT_MODE==0) attachInterrupt(interruptPin,wakeUp, LOW);
    //if(INTERRUPT_MODE==1) attachInterrupt(interruptPin,wakeUp, RISING);
    //if(INTERRUPT_MODE==2) attachInterrupt(interruptPin,wakeUp, FALLING);
    //if(INTERRUPT_MODE==3) attachInterrupt(interruptPin,wakeUp, CHANGE);
  }

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  WDTCR |= _BV(WDIE);
#else
  WDTCSR |= _BV(WDIE);
#endif

  while(sleep_cycles_remaining){
    sleep_mode();                        // System sleeps here
  }                                     // The WDT_vect interrupt wakes the MCU from here
  sleep_disable();                     // System continues execution here when watchdog timed out
  detachInterrupt(interruptPin);

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    WDTCR &= ~_BV(WDIE);
#else
    WDTCSR &= ~_BV(WDIE);
#endif
  return !wasInterrupted;
}

void RF24Network::setup_watchdog(uint8_t prescalar){

  uint8_t wdtcsr = prescalar & 7;
  if ( prescalar & 8 )
    wdtcsr |= _BV(WDP3);
  MCUSR &= ~_BV(WDRF);                      // Clear the WD System Reset Flag

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  WDTCR = _BV(WDCE) | _BV(WDE);            // Write the WD Change enable bit to enable changing the prescaler and enable system reset
  WDTCR = _BV(WDCE) | wdtcsr | _BV(WDIE);  // Write the prescalar bits (how long to sleep, enable the interrupt to wake the MCU
#else
  WDTCSR = _BV(WDCE) | _BV(WDE);            // Write the WD Change enable bit to enable changing the prescaler and enable system reset
  WDTCSR = _BV(WDCE) | wdtcsr | _BV(WDIE);  // Write the prescalar bits (how long to sleep, enable the interrupt to wake the MCU
#endif
}


#endif // not ATTiny
#endif // Enable sleep mode
