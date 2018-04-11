/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "nRF24L01.h"
#include "RF24_config.h"
#include "RF24.h"
#include "MCUClock.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

/****************************************************************************/

uint32_t RF24::millis()
{
    return mcuClock.millis();
}


void RF24::csn(int mode)
{
}

/****************************************************************************/

void RF24::ce(int level)
{
    if (level==HIGH)
    {
        rf24->setCE_HIGH();
    }
    else
    {
        rf24->setCE_LOW();
    }
}


/****************************************************************************/

uint8_t RF24::read_register(uint8_t reg, uint8_t* buf, uint8_t len)
{
    byte sentCMD[1] = {(byte) (R_REGISTER|(REGISTER_MASK&reg))};
    return rf24->Spi_Write(sentCMD, sizeof(byte), buf, len);
}

/****************************************************************************/

void RF24::reUseTX()
{
    //TODO:
    /*
    write_register(STATUS, _BV(MAX_RT));              //Clear max retry flag
    spiTrans(REUSE_TX_PL);
    ce(LOW);                                          //Re-Transfer packet
    ce(HIGH);
     */
}

/****************************************************************************/

uint8_t RF24::read_register(uint8_t reg)
{
    byte sentCMD[1] = {(byte) (R_REGISTER|(REGISTER_MASK&reg))};
    byte msgBack[1];
    rf24->Spi_Write(sentCMD, 1, msgBack);
    //printf("%s RF24::read_register reg:0x%X " BYTE_TO_BINARY_PATTERN "\n",rf24->LOGHDR, reg,BYTE_TO_BINARY(msgBack[0]));
    return msgBack[0];
}

/****************************************************************************/

void RF24::setAddressWidth(uint8_t a_width)
{

    if (a_width -= 2)
    {
        write_register(SETUP_AW, a_width % 4);
        addr_width = (a_width % 4) + 2;
    }
    else
    {
        write_register(SETUP_AW, 0);
        addr_width = 2;
    }

}

/****************************************************************************/

uint8_t RF24::write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
    byte* tempBuf = (byte*) calloc(sizeof(byte), len + 2);
    byte placeholder[5];
    memcpy(tempBuf + 1, buf, len);
    tempBuf[0] = W_REGISTER|(REGISTER_MASK&reg);

    return rf24->Spi_Write(tempBuf, len, placeholder);
}

/****************************************************************************/

uint8_t RF24::write_register(uint8_t reg, uint8_t value)
{
    //printf("%s RF24::write_register reg:0x%X value:0x%X\n",rf24->LOGHDR,reg,value);
    byte CMDsent[3] = {(byte) (W_REGISTER|(REGISTER_MASK&reg)), value, 0};
    byte placeholder[5];
    return rf24->Spi_Write(CMDsent, 1, placeholder);
}

/****************************************************************************/

uint8_t RF24::write_payload(const void* buf, uint8_t len)
{
    byte* tempBuf = (byte*) calloc(sizeof(byte), len + 2);
    byte placeholder[5];
    memcpy(tempBuf + 1, buf, len);
    tempBuf[0] = W_TX_PAYLOAD;

    return rf24->Spi_Write(tempBuf, len, placeholder);
}

/****************************************************************************/

uint8_t RF24::read_payload(void* buf, uint8_t len)
{
    byte CMDsent[2] = {R_RX_PAYLOAD, 0};
    return rf24->Spi_Write(CMDsent, 1, (byte*) buf, len);
}

/****************************************************************************/

uint8_t RF24::flush_rx(void)
{
    byte CMDsent[2] = {FLUSH_RX, 0};
    byte placeholder[5];
    return rf24->Spi_Write(CMDsent, 1, placeholder);
}

/****************************************************************************/

uint8_t RF24::flush_tx(void)
{
    byte CMDsent[2] = {FLUSH_TX, 0};
    byte placeholder[5];
    return rf24->Spi_Write(CMDsent, 1, placeholder);
}

/****************************************************************************/

uint8_t RF24::get_status(void)
{
    byte CMDsent[2] = {NOP, 0};
    byte placeholder[5];
    return rf24->Spi_Write(CMDsent, 1, placeholder);
}

/****************************************************************************/

void RF24::print_status(uint8_t status)
{
    printf("STATUS\t\t = 0x%02x RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\r\n",
           status,
           (status&_BV(RX_DR)) ? 1 : 0,
           (status&_BV(TX_DS)) ? 1 : 0,
           (status&_BV(MAX_RT)) ? 1 : 0,
           ((status >> RX_P_NO)&0b111),
           (status&_BV(TX_FULL_STATUS)) ? 1 : 0
    );
}

/****************************************************************************/

void RF24::print_observe_tx(uint8_t value)
{
    printf("OBSERVE_TX=%02x: POLS_CNT=%x ARC_CNT=%x\r\n",
           value,
           (value >> PLOS_CNT)&0b1111,
           (value >> ARC_CNT)&0b1111
    );
}

/****************************************************************************/

void RF24::print_byte_register(const char* name, uint8_t reg, uint8_t qty)
{
    printf("%s =", name);
    while (qty--)
    {
        printf(" 0x%02x", read_register(reg++));
    }
    printf("\r\n");
}

/****************************************************************************/

void RF24::print_address_register(const char* name, uint8_t reg, uint8_t qty)
{
    printf("%s =", name);

    int addrsize=5;
    while (qty--)
    {
        uint8_t buffer[sizeof(uint64_t)];
        read_register(reg++, buffer, sizeof(buffer));

        printf(" 0x");
        uint8_t* bufptr = buffer + sizeof(buffer)-(sizeof(buffer)-addrsize);
        while (--bufptr>=buffer)
        {
            printf("%02x", *bufptr);
        }
    }

    printf("\r\n");
}

/****************************************************************************/

RF24::RF24(uint8_t _cepin, uint8_t _cspin, nRF24l01plus* theNRFsim, MCUClock& _mcuClock) :
        ce_pin(_cepin), csn_pin(_cspin), wide_band(true), p_variant(false),
        payload_size(32), dynamic_payloads_enabled(false),
        pipe0_reading_address(0), rf24(theNRFsim), mcuClock(_mcuClock)
{
}

/****************************************************************************/

void RF24::setChannel(uint8_t channel)
{
    // TODO: This method could take advantage of the 'wide_band' calculation
    // done in setChannel() to require certain channel spacing.
    const uint8_t max_channel = 127;
    write_register(RF_CH, std::min(channel, max_channel));
}

/****************************************************************************/

void RF24::setPayloadSize(uint8_t size)
{
    const uint8_t max_payload_size = 32;
    payload_size = std::min(size, max_payload_size);
    enableDynamicPayloads();
}

/****************************************************************************/

uint8_t RF24::getPayloadSize(void)
{
    return payload_size;
}

/****************************************************************************/

static const char rf24_datarate_e_str_0[] = "1MBPS";
static const char rf24_datarate_e_str_1[] = "2MBPS";
static const char rf24_datarate_e_str_2[] = "250KBPS";
static const char* const rf24_datarate_e_str_P[] = {
        rf24_datarate_e_str_0,
        rf24_datarate_e_str_1,
        rf24_datarate_e_str_2,
};
static const char rf24_model_e_str_0[] = "nRF24L01";
static const char rf24_model_e_str_1[] = "nRF24L01+";
static const char* const rf24_model_e_str_P[] = {
        rf24_model_e_str_0,
        rf24_model_e_str_1,
};
static const char rf24_crclength_e_str_0[] = "Disabled";
static const char rf24_crclength_e_str_1[] = "8 bits";
static const char rf24_crclength_e_str_2[] = "16 bits";
static const char* const rf24_crclength_e_str_P[] = {
        rf24_crclength_e_str_0,
        rf24_crclength_e_str_1,
        rf24_crclength_e_str_2,
};
static const char rf24_pa_dbm_e_str_0[] = "PA_MIN";
static const char rf24_pa_dbm_e_str_1[] = "PA_LOW";
static const char rf24_pa_dbm_e_str_2[] = "LA_MED";
static const char rf24_pa_dbm_e_str_3[] = "PA_HIGH";
static const char* const rf24_pa_dbm_e_str_P[] = {
        rf24_pa_dbm_e_str_0,
        rf24_pa_dbm_e_str_1,
        rf24_pa_dbm_e_str_2,
        rf24_pa_dbm_e_str_3,
};

void RF24::dumpRegisters(const char* label)
{
    if (label!=nullptr)
    {
        printf("%s",label);
    }
    print_address_register("RX_ADDR_P0-1\t", RX_ADDR_P0, 2);
    print_byte_register("RX_ADDR_P2-5\t", RX_ADDR_P2, 4);
    print_address_register("TX_ADDR\t\t", TX_ADDR);
    //print_byte_register(PSTR("RX_PW_P0-6"), RX_PW_P0, 6);
    print_byte_register("EN_AA\t\t", EN_AA);
    //print_byte_register(PSTR("EN_RXADDR"), EN_RXADDR);
    //print_byte_register(PSTR("RF_SETUP"), RF_SETUP);
    //print_byte_register(PSTR("CONFIG\t"), NRF_CONFIG);
    //print_byte_register(PSTR("DYNPD/FEATURE"), DYNPD, 2);
}

void RF24::printDetails(void)
{
    /*print_status(get_status());

    print_address_register(PSTR("RX_ADDR_P0-1"),RX_ADDR_P0,2);
    print_byte_register(PSTR("RX_ADDR_P2-5"),RX_ADDR_P2,4);
    print_address_register(PSTR("TX_ADDR"),TX_ADDR);

    print_byte_register(PSTR("RX_PW_P0-6"),RX_PW_P0,6);
    print_byte_register(PSTR("EN_AA"),EN_AA);
    print_byte_register(PSTR("EN_RXADDR"),EN_RXADDR);
    print_byte_register(PSTR("RF_CH"),RF_CH);
    print_byte_register(PSTR("RF_SETUP"),RF_SETUP);
    print_byte_register(PSTR("CONFIG"),CONFIG);
    print_byte_register(PSTR("DYNPD/FEATURE"),DYNPD,2);

    printf_P(PSTR("Data Rate\t = %S\r\n"),pgm_read_word(&rf24_datarate_e_str_P[getDataRate()]));
    printf_P(PSTR("Model\t\t = %S\r\n"),pgm_read_word(&rf24_model_e_str_P[isPVariant()]));
    printf_P(PSTR("CRC Length\t = %S\r\n"),pgm_read_word(&rf24_crclength_e_str_P[getCRCLength()]));
    printf_P(PSTR("PA Power\t = %S\r\n"),pgm_read_word(&rf24_pa_dbm_e_str_P[getPALevel()]));
  */
    rf24->printRegContents();
}

/****************************************************************************/

void RF24::begin()
{

    uint8_t setup = 0;

    ce(LOW);
    csn(HIGH);

    // Must allow the radio time to settle else configuration bits will not necessarily stick.
    // This is actually only required following power up but some settling time also appears to
    // be required after resets too. For full coverage, we'll always assume the worst.
    // Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
    // Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
    // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
    usleep(5000);

    // Reset NRF_CONFIG and enable 16-bit CRC.
    write_register(CONFIG, 0x0C);

    // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
    // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
    // sizes must never be used. See documentation for a more complete explanation.
    setRetries(5, 15);

    // Determine if this is a p or non-p RF24 module and then
    // reset our data rate back to default value. This works
    // because a non-P variant won't allow the data rate to
    // be set to 250Kbps.
    if (setDataRate(RF24_250KBPS))
    {
        p_variant = true;
    }

    // Then set the data rate to the slowest (and most reliable) speed supported by all
    // hardware.
    setDataRate(RF24_1MBPS);

    // Initialize CRC and request 2-byte (16bit) CRC
    //setCRCLength(RF24_CRC_16);


    // Disable dynamic payloads, to match dynamic_payloads_enabled setting - Reset value is 0
    toggle_features();
    write_register(FEATURE, 0);
    write_register(DYNPD, 0);
    dynamic_payloads_enabled = false;

    // Reset current status
    // Notice reset and flush is the last thing we do
    write_register(STATUS, _BV(RX_DR)|_BV(TX_DS)|_BV(MAX_RT));

    // Set up default configuration.  Callers can always change it later.
    // This channel should be universally safe and not bleed over into adjacent
    // spectrum.
    setChannel(76);


    // Flush buffers
    flush_rx();
    flush_tx();

    powerUp(); //Power up by default when begin() is called

    // Enable PTX, do not write CE high so radio will remain in standby I mode ( 130us max to transition to RX or TX instead of 1500us from powerUp )
    // PTX should use only 22uA of power
    write_register(CONFIG, (read_register(CONFIG))&~_BV(PRIM_RX));

    // if setup is 0 or ff then there was no response from module
}

/****************************************************************************/

void RF24::startListening()
{
    write_register(CONFIG, read_register(CONFIG)|_BV(PWR_UP)|_BV(PRIM_RX));
    write_register(STATUS, _BV(RX_DR)|_BV(TX_DS)|_BV(MAX_RT));

    ce(HIGH);
    // Restore the pipe0 adddress, if exists
    if (pipe0_reading_address)
    {
        write_register(RX_ADDR_P0, reinterpret_cast<const uint8_t*>(&pipe0_reading_address), 5);
    }
    else
    {
        closeReadingPipe(0);
    }

    // Flush buffers
    //flush_rx();
    if (read_register(FEATURE)&_BV(EN_ACK_PAY))
    {
        flush_tx();
    }

    // wait for the radio to come up (130us actually only needed)
    usleep(130);
}

/****************************************************************************/

bool RF24::rxFifoFull()
{
    return read_register(FIFO_STATUS)&_BV(RX_FULL);
}

/****************************************************************************/

void RF24::stopListening(void)
{
    ce(LOW);

    usleep(txDelay);

    if (read_register(FEATURE)&_BV(EN_ACK_PAY))
    {
        usleep(txDelay); //200
        flush_tx();
    }

    write_register(CONFIG, (read_register(CONFIG))&~_BV(PRIM_RX));

    write_register(EN_RXADDR, read_register(EN_RXADDR)|_BV(pgm_read_byte(&child_pipe_enable[0]))); // Enable RX on pipe0
}

/****************************************************************************/

void RF24::powerDown(void)
{
    ce(LOW);
    write_register(CONFIG, read_register(CONFIG)&~_BV(PWR_UP));
}

/****************************************************************************/

void RF24::powerUp(void)
{
    uint8_t cfg = read_register(CONFIG);

    // if not powered up then power up and wait for the radio to initialize
    if (!(cfg&_BV(PWR_UP)))
    {
        write_register(CONFIG, cfg|_BV(PWR_UP));

        // For nRF24L01+ to go from power down mode to TX or RX mode it must first pass through stand-by mode.
        // There must be a delay of Tpd2stby (see Table 16.) after the nRF24L01+ leaves power down mode before
        // the CEis set high. - Tpd2stby can be up to 5ms per the 1.0 datasheet
        usleep(5000);
    }
}

/******************************************************************/

void RF24::closeReadingPipe(uint8_t pipe)
{
    write_register(EN_RXADDR, read_register(EN_RXADDR)&~_BV(child_pipe_enable[pipe]));
}

bool RF24::write(const void* buf, uint8_t len)
{
    return write(buf,len,false);
}

bool RF24::write(const void* buf, uint8_t len,bool multicast)
{
    printf("%d RF24::write len=%u\n", rf24->id, len);

        //Start Writing
        startFastWrite(buf, len, multicast);

        //Wait until complete or failed
        uint32_t timer = millis();

        while (!(get_status()&(_BV(TX_DS)|_BV(MAX_RT))))
        {
            if(millis() - timer > 95)
            {
                return false;
            }
        }

        ce(LOW);

        uint8_t status = write_register(STATUS, _BV(RX_DR)|_BV(TX_DS)|_BV(MAX_RT));

        //Max retries exceeded
        if (status&_BV(MAX_RT))
        {
            flush_tx(); //Only going to be 1 packet int the FIFO at a time using this method, so just flush
            return 0;
        }
        //TX OK 1 or 0
        return 1;
}

/****************************************************************************/

void RF24::startWrite(const void* buf, uint8_t len)
{
    // Transmitter power-up
    write_register(CONFIG, (read_register(CONFIG)|_BV(PWR_UP))&~_BV(PRIM_RX));
    usleep(150);

    // Send the payload
    write_payload(buf, len);

    // Allons!
    ce(HIGH);
    usleep(15);
    ce(LOW);
}

/****************************************************************************/

uint8_t RF24::getDynamicPayloadSize()
{
    byte sentCMD[2] = {R_RX_PL_WID, 0};
    byte msgBack[1];
    rf24->Spi_Write(sentCMD, 1, msgBack);
    return msgBack[0];
}

/****************************************************************************/

bool RF24::available()
{
    return available(nullptr);
}

/****************************************************************************/

bool RF24::available(uint8_t* pipe_num)
{
    if (!(read_register(FIFO_STATUS)&_BV(RX_EMPTY)))
    {

        // If the caller wants the pipe number, include that
        if (pipe_num)
        {
            uint8_t status = get_status();
            *pipe_num = (status >> RX_P_NO)&0x07;
        }
        return 1;
    }

    return 0;
}

/****************************************************************************/

void RF24::read(void* buf, uint8_t len)
{

    // Fetch the payload
    read_payload(buf, len);

    //Clear the two possible interrupt flags with one command
    write_register(STATUS, _BV(RX_DR)|_BV(MAX_RT)|_BV(TX_DS));

}

/****************************************************************************/

void RF24::whatHappened(bool& tx_ok, bool& tx_fail, bool& rx_ready)
{

    // Read the status & reset the status in one easy call
    // Or is that such a good idea?
    uint8_t status = write_register(STATUS, _BV(RX_DR)|_BV(TX_DS)|_BV(MAX_RT));

    // Report to the user what happened
    tx_ok = status&_BV(TX_DS);
    tx_fail = status&_BV(MAX_RT);
    rx_ready = status&_BV(RX_DR);

    printf("%s whatHappened txOk=%u, txFail=%u, rxReady=%u\n",rf24->LOGHDR,tx_ok,tx_fail,rx_ready);
}

/****************************************************************************/

void RF24::openWritingPipe(uint64_t value)
{
    // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01(+)
    // expects it LSB first too, so we're good.

    printf("%s wpipe   0x%llx\n",rf24->LOGHDR,value);


    write_register(RX_ADDR_P0, reinterpret_cast<uint8_t*>(&value), 5);
    write_register(TX_ADDR, reinterpret_cast<uint8_t*>(&value), 5);

    const uint8_t max_payload_size = 32;
    write_register(RX_PW_P0, std::min(payload_size, max_payload_size));
}

/****************************************************************************/



void RF24::openReadingPipe(uint8_t child, uint64_t address)
{
    //printf("%s rpipe %u:0x%llx\n",rf24->LOGHDR,child,address);
    // If this is pipe 0, cache the address.  This is needed because
    // openWritingPipe() will overwrite the pipe 0 address, so
    // startListening() will have to restore it.
    if (child==0)
    {
        pipe0_reading_address = address;
    }

    if (child<=6)
    {
        // For pipes 2-5, only write the LSB
        if (child<2)
        {
            write_register(child_pipe[child], reinterpret_cast<const uint8_t*>(&address), 5);
        }
        else
        {
            write_register(child_pipe[child], reinterpret_cast<const uint8_t*>(&address), 1);
        }

        write_register(child_payload_size[child], payload_size);

        // Note it would be more efficient to set all of the bits for all open
        // pipes at once.  However, I thought it would make the calling code
        // more simple to do it this way.
        write_register(EN_RXADDR, read_register(EN_RXADDR)|_BV(child_pipe_enable[child]));
    }
}

/****************************************************************************/

void RF24::toggle_features()
{
    byte sentCMD[3] = {ACTIVATE, 0x73, 0};
    byte msgBack[1];
    rf24->Spi_Write(sentCMD, 1, msgBack);
}

/****************************************************************************/

void RF24::enableDynamicPayloads()
{
    // Enable dynamic payload throughout the system

    //toggle_features();
    write_register(FEATURE, read_register(FEATURE)|_BV(EN_DPL));


    // Enable dynamic payload on all pipes
    //
    // Not sure the use case of only having dynamic payload on certain
    // pipes, so the library does not support it.
    write_register(DYNPD, read_register(DYNPD)|_BV(DPL_P5)|_BV(DPL_P4)|_BV(DPL_P3)|_BV(DPL_P2)|_BV(DPL_P1)|_BV(DPL_P0));

    dynamic_payloads_enabled = true;
}


/****************************************************************************/
void RF24::disableDynamicPayloads()
{
    // Disables dynamic payload throughout the system.  Also disables Ack Payloads

    //toggle_features();
    write_register(FEATURE, read_register(FEATURE)&~(_BV(EN_DPL)|_BV(EN_ACK_PAY)));

    // Disable dynamic payload on all pipes
    //
    // Not sure the use case of only having dynamic payload on certain
    // pipes, so the library does not support it.
    write_register(DYNPD, read_register(DYNPD)&~(_BV(DPL_P5)|_BV(DPL_P4)|_BV(DPL_P3)|_BV(DPL_P2)|_BV(DPL_P1)|_BV(DPL_P0)));

    dynamic_payloads_enabled = false;
}

/****************************************************************************/

void RF24::enableAckPayload()
{
    //
    // enable ack payload and dynamic payload features
    //
    write_register(FEATURE, read_register(FEATURE)|_BV(EN_ACK_PAY)|_BV(EN_DPL));


    //
    // Enable dynamic payload on pipes 0 & 1
    //
    write_register(DYNPD, read_register(DYNPD)|_BV(DPL_P1)|_BV(DPL_P0));
    dynamic_payloads_enabled = true;
}

/****************************************************************************/

void RF24::writeAckPayload(uint8_t pipe, const void* buf, uint8_t len)
{
    printf("%s writeAckPayload pipe=%u\n",rf24->LOGHDR,pipe);
    byte* tempBuf = (byte*) calloc(sizeof(byte), len + 2);
    byte placeholder[5];
    memcpy(tempBuf + 1, buf, len);
    tempBuf[0] = W_ACK_PAYLOAD|(pipe&0b111);

    rf24->Spi_Write(tempBuf, len, placeholder);
}

/****************************************************************************/

void RF24::enableDynamicAck()
{
    //
    // enable dynamic ack features
    //
    write_register(FEATURE, read_register(FEATURE)|_BV(EN_DYN_ACK));
}

/****************************************************************************/

bool RF24::isAckPayloadAvailable()
{
    return !(read_register(FIFO_STATUS)&_BV(RX_EMPTY));
}

/****************************************************************************/

bool RF24::isPVariant()
{
    return p_variant;
}

/****************************************************************************/

void RF24::setAutoAck(bool enable)
{
    if (enable)
    {
        write_register(EN_AA, 0b111111);
    }
    else
    {
        write_register(EN_AA, 0);
    }
}

/****************************************************************************/

void RF24::setAutoAck(uint8_t pipe, bool enable)
{
    if (pipe<=6)
    {
        uint8_t en_aa = read_register(EN_AA);
        if (enable)
        {
            en_aa |= _BV(pipe);
        }
        else
        {
            en_aa &= ~_BV(pipe);
        }
        write_register(EN_AA, en_aa);
    }
}

/****************************************************************************/

bool RF24::testCarrier()
{
    return (read_register(CD)&1);
}

/****************************************************************************/

bool RF24::testRPD()
{
    return (read_register(RPD)&1);
}

/****************************************************************************/

void RF24::setPALevel(rf24_pa_dbm_e level)
{
    uint8_t setup = read_register(RF_SETUP);
    setup &= ~(_BV(RF_PWR_LOW)|_BV(RF_PWR_HIGH));

    // switch uses RAM (evil!)
    if (level==RF24_PA_MAX)
    {
        setup |= (_BV(RF_PWR_LOW)|_BV(RF_PWR_HIGH));
    }
    else if (level==RF24_PA_HIGH)
    {
        setup |= _BV(RF_PWR_HIGH);
    }
    else if (level==RF24_PA_LOW)
    {
        setup |= _BV(RF_PWR_LOW);
    }
    else if (level==RF24_PA_MIN)
    {
        // nothing
    }
    else if (level==RF24_PA_ERROR)
    {
        // On error, go to maximum PA
        setup |= (_BV(RF_PWR_LOW)|_BV(RF_PWR_HIGH));
    }

    write_register(RF_SETUP, setup);
}

/****************************************************************************/

rf24_pa_dbm_e RF24::getPALevel()
{
    rf24_pa_dbm_e result = RF24_PA_ERROR;
    uint8_t power = read_register(RF_SETUP)&(_BV(RF_PWR_LOW)|_BV(RF_PWR_HIGH));

    // switch uses RAM (evil!)
    if (power==(_BV(RF_PWR_LOW)|_BV(RF_PWR_HIGH)))
    {
        result = RF24_PA_MAX;
    }
    else if (power==_BV(RF_PWR_HIGH))
    {
        result = RF24_PA_HIGH;
    }
    else if (power==_BV(RF_PWR_LOW))
    {
        result = RF24_PA_LOW;
    }
    else
    {
        result = RF24_PA_MIN;
    }

    return result;
}

/****************************************************************************/

//For general use, the interrupt flags are not important to clear
bool RF24::writeBlocking(const void* buf, uint8_t len, uint32_t timeout)
{
    //Block until the FIFO is NOT full.
    //Keep track of the MAX retries and set auto-retry if seeing failures
    //This way the FIFO will fill up and allow blocking until packets go through
    //The radio will auto-clear everything in the FIFO as long as CE remains high

    uint32_t timer = millis();                              //Get the time that the payload transmission started

    while ((get_status()&(_BV(TX_FULL_STATUS))))
    {
        //Blocking only if FIFO is full. This will loop and block until TX is successful or timeout
        if (get_status()&_BV(MAX_RT))
        {                      //If MAX Retries have been reached
            reUseTX();                                          //Set re-transmit and clear the MAX_RT interrupt flag
            if (millis() - timer>timeout)
            {
                return 0;
            }          //If this payload has exceeded the user-defined timeout, exit and return 0
        }
#if defined (FAILURE_HANDLING) || defined (RF24_LINUX)
        if(millis() - timer > (timeout+95) )
        {
            errNotify();
   #if defined (FAILURE_HANDLING)
            return 0;
   #endif
        }
#endif

    }

    //Start Writing
    startFastWrite(buf, len, 0);                                  //Write the payload if a buffer is clear

    return 1;                                                  //Return 1 to indicate successful transmission
}

/****************************************************************************/

bool RF24::setDataRate(rf24_datarate_e speed)
{
    bool result = false;
    uint8_t setup = read_register(RF_SETUP);

    // HIGH and LOW '00' is 1Mbs - our default
    wide_band = false;
    setup &= ~(_BV(RF_DR_LOW)|_BV(RF_DR_HIGH));
    txDelay = 85;
    if (speed==RF24_250KBPS)
    {
        // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
        // Making it '10'.
        wide_band = false;
        setup |= _BV(RF_DR_LOW);
        txDelay = 450;
    }
    else
    {
        // Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
        // Making it '01'
        if (speed==RF24_2MBPS)
        {
            wide_band = true;
            setup |= _BV(RF_DR_HIGH);
            txDelay = 190;
        }
        else
        {
            // 1Mbs
            wide_band = false;
        }
    }
    write_register(RF_SETUP, setup);

    // Verify our result
    if (read_register(RF_SETUP)==setup)
    {
        result = true;
    }
    else
    {
        wide_band = false;
    }

    return result;
}

/****************************************************************************/

rf24_datarate_e RF24::getDataRate()
{
    rf24_datarate_e result;
    uint8_t dr = read_register(RF_SETUP)&(_BV(RF_DR_LOW)|_BV(RF_DR_HIGH));

    // switch uses RAM (evil!)
    // Order matters in our case below
    if (dr==_BV(RF_DR_LOW))
    {
        // '10' = 250KBPS
        result = RF24_250KBPS;
    }
    else if (dr==_BV(RF_DR_HIGH))
    {
        // '01' = 2MBPS
        result = RF24_2MBPS;
    }
    else
    {
        // '00' = 1MBPS
        result = RF24_1MBPS;
    }
    return result;
}

/****************************************************************************/

void RF24::setCRCLength(rf24_crclength_e length)
{
    uint8_t config = read_register(CONFIG)&~(_BV(CRCO)|_BV(EN_CRC));

    // switch uses RAM (evil!)
    if (length==RF24_CRC_DISABLED)
    {
        // Do nothing, we turned it off above.
    }
    else if (length==RF24_CRC_8)
    {
        config |= _BV(EN_CRC);
    }
    else
    {
        config |= _BV(EN_CRC);
        config |= _BV(CRCO);
    }
    write_register(CONFIG, config);
}

/****************************************************************************/

rf24_crclength_e RF24::getCRCLength()
{
    rf24_crclength_e result = RF24_CRC_DISABLED;
    uint8_t config = read_register(CONFIG)&(_BV(CRCO)|_BV(EN_CRC));

    if (config&_BV(EN_CRC))
    {
        if (config&_BV(CRCO))
        {
            result = RF24_CRC_16;
        }
        else
        {
            result = RF24_CRC_8;
        }
    }

    return result;
}

/****************************************************************************/

void RF24::disableCRC()
{
    uint8_t disable = read_register(CONFIG)&~_BV(EN_CRC);
    write_register(CONFIG, disable);
}

/****************************************************************************/
void RF24::setRetries(uint8_t delay, uint8_t count)
{
    write_register(SETUP_RETR, (delay&0xf) << ARD|(count&0xf) << ARC);
}

bool RF24::txStandBy(uint32_t timeout, bool startTx)
{
    printf("%s txStandby timeout=%u\n",rf24->LOGHDR,timeout);
    if (startTx)
    {
        stopListening();
        ce(HIGH);
    }

    int yield = 0;

    uint32_t start = millis();

    //TODO: SIM ONLY
    //This exits when the ack is received
    rf24->waitForTX(timeout);

    while (!(read_register(FIFO_STATUS)&_BV(TX_EMPTY)))
    {
        if (millis() - start>=timeout)
        {
            printf("%s txStandby timeout waited for %u\n",rf24->LOGHDR,timeout);
            ce(LOW);
            flush_tx();
            return 0;
        }

        if (get_status()&_BV(MAX_RT))
        {
            //Set re-transmit
            write_register(STATUS, _BV(MAX_RT));
            ce(LOW);
            ce(HIGH);
        }

        YIELDAT(200);
    }

    ce(LOW);                   //Set STANDBY-I mode
    return 1;

}

bool RF24::writeFast(const void* buf, uint8_t len, const bool multicast)
{
    printf("%s writeFast\n",rf24->LOGHDR);
    //Block until the FIFO is NOT full.
    //Keep track of the MAX retries and set auto-retry if seeing failures
    //Return 0 so the user can control the retrys and set a timer or failure counter if required
    //The radio will auto-clear everything in the FIFO as long as CE remains high

    uint32_t timer = millis();

    //Blocking only if FIFO is full. This will loop and block until TX is successful or fail
    while ((get_status()&(_BV(TX_FULL_STATUS))))
    {
        if (get_status()&_BV(MAX_RT))
        {
            //reUseTX();							//Set re-transmit
            write_register(STATUS, _BV(MAX_RT));    //Clear max retry flag
            return false;                               //Return 0. The previous payload has been retransmitted
            //From the user perspective, if you get a 0, just keep trying to send the same payload
        }
        if (millis() - timer > write_fast_timeout )
        {
            printf("%s fastWrite txfifo full for timeout=%u\n",rf24->LOGHDR,write_fast_timeout);
            return false;
        }
    }
    //Start Writing
    startFastWrite(buf, len, multicast);

    return true;
}

bool RF24::writeFast(const void* buf, uint8_t len)
{
    return writeFast(buf, len, 0);
}

/****************************************************************************/

//Per the documentation, we want to set PTX Mode when not listening. Then all we do is write data and set CE high
//In this mode, if we can keep the FIFO buffers loaded, packets will transmit immediately (no 130us delay)
//Otherwise we enter Standby-II mode, which is still faster than standby mode
//Also, we remove the need to keep writing the config register over and over and delaying for 150 us each time if sending a stream of data

void RF24::startFastWrite(const void* buf, uint8_t len, const bool multicast, bool startTx)
{ //TMRh20

    printf("%s startFastWrite\n",rf24->LOGHDR);
    //write_payload( buf,len);
    write_payload(buf, len, multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD);
    if (startTx)
    {
        ce(HIGH);
    }
}

uint8_t RF24::write_payload(const void* buf, uint8_t data_len, const uint8_t writeType)
{
    printf("%s write_payload\n",rf24->LOGHDR);
    uint8_t status;
    const uint8_t* current = reinterpret_cast<const uint8_t*>(buf);

    data_len = rf24_min(data_len, payload_size);
    uint8_t blank_len = dynamic_payloads_enabled ? 0 : payload_size - data_len;

    //printf("Writing %u bytes %u blanks\n", data_len, blank_len);

    //Safer, was a memory leak
    auto bufCopy = shared_ptr<byte>(new byte[data_len + 2]);
    auto tempBuf = bufCopy.get();

    byte placeholder[5];
    tempBuf[0] = writeType;
    memcpy(tempBuf + 1, buf, data_len);

    return rf24->Spi_Write(tempBuf, data_len, placeholder);
}

// vim:ai:cin:sts=2 sw=2 ft=cpp

