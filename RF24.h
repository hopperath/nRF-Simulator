/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * @file RF24.h
 *
 * Class declaration for RF24 and helper enums
 */

#ifndef __RF24_H__
#define __RF24_H__

#include "RF24_config.h"
#include "nRF24l01plus.h"
#include "nRF24L01.h"

#define LOW 0
#define HIGH 1
#define TX_STANDBY_TIMEOUT 750

static const uint8_t child_pipe[] = { RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5 };
static const uint8_t child_payload_size[] = { RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5 };
static const uint8_t child_pipe_enable[] = { ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5 };

/**
 * Power Amplifier level.
 *
 * For use with setPALevel()
 */
typedef enum
{
    RF24_PA_MIN = 0, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX, RF24_PA_ERROR
} rf24_pa_dbm_e;

/**
 * Data rate.  How fast data moves through the air.
 *
 * For use with setDataRate()
 */
typedef enum
{
    RF24_1MBPS = 0, RF24_2MBPS, RF24_250KBPS
} rf24_datarate_e;

/**
 * CRC Length.  How big (if any) of a CRC is included.
 *
 * For use with setCRCLength()
 */
typedef enum
{
    RF24_CRC_DISABLED = 0, RF24_CRC_8, RF24_CRC_16
} rf24_crclength_e;

/**
 * Driver for nRF24L01(+) 2.4GHz Wireless Transceiver
 */

class RF24
{
    private:
    uint8_t ce_pin; /**< "Chip Enable" pin, activates the RX or TX role */
    uint8_t csn_pin; /**< SPI Chip select */
    bool wide_band; /* 2Mbs data rate in use? */
    bool p_variant; /* False for RF24L01 and true for RF24L01P */
    uint8_t payload_size; /**< Fixed size of payloads */
    bool dynamic_payloads_enabled; /**< Whether dynamic payloads are enabled. */
    uint8_t ack_payload_length; /**< Dynamic size of pending ack payload. */
    uint64_t pipe0_reading_address; /**< Last address set on pipe 0 for reading. */
    uint8_t addr_width; /**< The address width to use - 3,4 or 5 bytes. */
    nRF24l01plus* theNRF24l01plus;
    protected:

    /**
    *
    * Thriver will delay for this duration when stopListening() is called
    *
    * When responding to payloads, faster devices like ARM(RPi) are much faster than Arduino:
    * 1. Arduino sends data to RPi, switches to RX mode
    * 2. The RPi receives the data, switches to TX mode and sends before the Arduino radio is in RX mode
    * 3. If AutoACK is disabled, this can be set as low as 0. If AA/ESB enabled, set to 100uS minimum on RPi
    *
    * @warning If set to 0, ensure 130uS delay after stopListening() and before any sends
    */

    uint32_t txDelay;

    /**
     * @name Low-level internal interface.
     *
     *  Protected methods that address the chip directly.  Regular users cannot
     *  ever call these.  They are documented for completeness and for developers who
     *  may want to extend this class.
     */
    /**@{*/

    /**
     * Disable dynamically-sized payloads
     *
     * This disables dynamic payloads on ALL pipes. Since Ack Payloads
     * requires Dynamic Payloads, Ack Payloads are also disabled.
     * If dynamic payloads are later re-enabled and ack payloads are desired
     * then enableAckPayload() must be called again as well.
     *
     */
    void disableDynamicPayloads();

    /**
     * Enable dynamic ACKs (single write multicast or unicast) for chosen messages
     *
     * @note To enable full multicast or per-pipe multicast, use setAutoAck()
     *
     * @warning This MUST be called prior to attempting single write NOACK calls
     * @code
     * radio.enableDynamicAck();
     * radio.write(&data,32,1);  // Sends a payload with no acknowledgement requested
     * radio.write(&data,32,0);  // Sends a payload using auto-retry/autoACK
     * @endcode
     */
    void enableDynamicAck();

    /**
     * This function is mainly used internally to take advantage of the auto payload
     * re-use functionality of the chip, but can be beneficial to users as well.
     *
     * The function will instruct the radio to re-use the data in the FIFO buffers,
     * and instructs the radio to re-send once the timeout limit has been reached.
     * Used by writeFast and writeBlocking to initiate retries when a TX failure
     * occurs. Retries are automatically initiated except with the standard write().
     * This way, data is not flushed from the buffer until switching between modes.
     *
     * @note This is to be used AFTER auto-retry fails if wanting to resend
     * using the built-in payload reuse features.
     * After issuing reUseTX(), it will keep reending the same payload forever or until
     * a payload is written to the FIFO, or a flush_tx command is given.
     */
    void reUseTX();

    /**
     * Check if the radio needs to be read. Can be used to prevent data loss
     * @return True if all three 32-byte radio buffers are full
     */
    bool rxFifoFull();

    /**
     * Set chip select pin
     *
     * Running SPI bus at PI_CLOCK_DIV2 so we don't waste time transferring data
     * and best of all, we make use of the radio's FIFO buffers. A lower speed
     * means we're less likely to effectively leverage our FIFOs and pay a higher
     * AVR runtime cost as toll.
     *
     * @param mode HIGH to take this unit off the SPI bus, LOW to put it on
     */
    void csn(int mode);

    /**
     * Set chip enable
     *
     * @param level HIGH to actively begin transmission or LOW to put in standby.  Please see data sheet
     * for a much more detailed description of this pin.
     */
    void ce(int level);

    /**
     * Read a chunk of data in from a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param buf Where to put the data
     * @param len How many bytes of data to transfer
     * @return Current value of status register
     */
    uint8_t read_register(uint8_t reg, uint8_t* buf, uint8_t len);

    /**
     * Read single byte from a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @return Current value of register @p reg
     */
    uint8_t read_register(uint8_t reg);

    /**
     * Set the address width from 3 to 5 bytes (24, 32 or 40 bit)
     *
     * @param a_width The address width to use: 3,4 or 5
     */
    void setAddressWidth(uint8_t a_width);

    /**
     * Write a chunk of data to a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param buf Where to get the data
     * @param len How many bytes of data to transfer
     * @return Current value of status register
     */
    uint8_t write_register(uint8_t reg, const uint8_t* buf, uint8_t len);

    /**
     * Write a single byte to a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param value The new value to write
     * @return Current value of status register
     */
    uint8_t write_register(uint8_t reg, uint8_t value);

    /**
     * Write the transmit payload
     *
     * The size of data written is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to get the data
     * @param len Number of bytes to be sent
     * @return Current value of status register
     */
    uint8_t write_payload(const void* buf, uint8_t len);

    /**
     * Write the transmit payload
     *
     * The size of data written is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to get the data
     * @param len Number of bytes to be sent
     * @return Current value of status register
     */
    uint8_t write_payload(const void* buf, uint8_t len, const uint8_t writeType);

    /**
     * Read the receive payload
     *
     * The size of data read is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to put the data
     * @param len Maximum number of bytes to read
     * @return Current value of status register
     */
    uint8_t read_payload(void* buf, uint8_t len);

    /**
     * Empty the receive buffer
     *
     * @return Current value of status register
     */
    uint8_t flush_rx(void);

    /**
     * Empty the transmit buffer
     *
     * @return Current value of status register
     */
    uint8_t flush_tx(void);

    /**
     * Retrieve the current status of the chip
     *
     * @return Current value of status register
     */
    uint8_t get_status(void);

    /**
     * Decode and print the given status to stdout
     *
     * @param status Status value to print
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void print_status(uint8_t status);

    /**
     * Decode and print the given 'observe_tx' value to stdout
     *
     * @param value The observe_tx value to print
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void print_observe_tx(uint8_t value);

    /**
     * Print the name and value of an 8-bit register to stdout
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param name Name of the register
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     */
    void print_byte_register(const char* name, uint8_t reg, uint8_t qty = 1);

    /**
     * Print the name and value of a 40-bit address register to stdout
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param name Name of the register
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     */
    void print_address_register(const char* name, uint8_t reg, uint8_t qty = 1);

    /**
     * Turn on or off the special features of the chip
     *
     * The chip has certain 'features' which are only available when the 'features'
     * are enabled.  See the datasheet for details.
     */
    void toggle_features(void);
    /**@}*/

    public:
    /**
     * @name Primary public interface
     *
     *  These are the main methods you need to operate the chip
     */
    /**@{*/

    /**
     * Constructor
     *
     * Creates a new instance of this driver.  Before using, you create an instance
     * and send in the unique pins that this chip is connected to.
     *
     * @param _cepin The pin attached to Chip Enable on the RF module
     * @param _cspin The pin attached to Chip Select
     */
    RF24(uint8_t _cepin, uint8_t _cspin, nRF24l01plus* theNRFsim);

    /**
     * Begin operation of the chip
     *
     * Call this in setup(), before calling any other methods.
     */
    void begin(void);

    /**
     * Start listening on the pipes opened for reading.
     *
     * Be sure to call openReadingPipe() first.  Do not call write() while
     * in this mode, without first calling stopListening().  Call
     * isAvailable() to check for incoming traffic, and read() to get it.
     */
    void startListening(void);

    /**
     * Stop listening for incoming messages
     *
     * Do this before calling write().
     */
    void stopListening(void);

    /**
     * Write to the open writing pipe
     *
     * Be sure to call openWritingPipe() first to set the destination
     * of where to write to.
     *
     * This blocks until the message is successfully acknowledged by
     * the receiver or the timeout/retransmit maxima are reached.  In
     * the current configuration, the max delay here is 60ms.
     *
     * The maximum size of data written is the fixed payload size, see
     * getPayloadSize().  However, you can write less, and the remainder
     * will just be filled with zeroes.
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @return True if the payload was delivered successfully false if not
     */
    bool write(const void* buf, uint8_t len);

    /**
     * Write for single NOACK writes. Optionally disables acknowledgements/autoretries for a single write.
     *
     * @note enableDynamicAck() must be called to enable this feature
     *
     * Can be used with enableAckPayload() to request a response
     * @see enableDynamicAck()
     * @see setAutoAck()
     * @see write()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param multicast Request ACK (0), NOACK (1)
     */
    bool write( const void* buf, uint8_t len, const bool multicast );

    /**
     * This function extends the auto-retry mechanism to any specified duration.
     * It will not block until the 3 FIFO buffers are filled with data.
     * If so the library will auto retry until a new payload is written
     * or the user specified timeout period is reached.
     * @warning It is important to never keep the nRF24L01 in TX mode and FIFO full for more than 4ms at a time. If the auto
     * retransmit is enabled, the nRF24L01 is never in TX mode long enough to disobey this rule. Allow the FIFO
     * to clear by issuing txStandBy() or ensure appropriate time between transmissions.
     *
     * @code
     * Example (Full blocking):
     *
     *			radio.writeBlocking(&buf,32,1000); //Wait up to 1 second to write 1 payload to the buffers
     *			txStandBy(1000);     			   //Wait up to 1 second for the payload to send. Return 1 if ok, 0 if failed.
     *					  				   		   //Blocks only until user timeout or success. Data flushed on fail.
     * @endcode
     * @note If used from within an interrupt, the interrupt should be disabled until completion, and sei(); called to enable millis().
     * @see txStandBy()
     * @see write()
     * @see writeFast()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @param timeout User defined timeout in milliseconds.
     * @return True if the payload was loaded into the buffer successfully false if not
     */
    bool writeBlocking( const void* buf, uint8_t len, uint32_t timeout );

    /**
     * Test whether there are bytes available to be read
     *
     * @return True if there is a payload available, false if none is
     */
    bool available(void);

    /**
     * Read the payload
     *
     * Return the last payload received
     *
     * The size of data read is the fixed payload size, see getPayloadSize()
     *
     * @note I specifically chose 'void*' as a data type to make it easier
     * for beginners to use.  No casting needed.
     *
     * @param buf Pointer to a buffer where the data should be written
     * @param len Maximum number of bytes to read into the buffer
     */
    void read(void* buf, uint8_t len);

    /**
     * Open a pipe for writing
     *
     * Only one pipe can be open at once, but you can change the pipe
     * you'll listen to.  Do not call this while actively listening.
     * Remember to stopListening() first.
     *
     * Addresses are 40-bit hex values, e.g.:
     *
     * @code
     *   openWritingPipe(0xF0F0F0F0F0);
     * @endcode
     *
     * @param address The 40-bit address of the pipe to open.  This can be
     * any value whatsoever, as long as you are the only one writing to it
     * and only one other radio is listening to it.  Coordinate these pipe
     * addresses amongst nodes on the network.
     */
    void openWritingPipe(uint64_t address);

    /**
     * Open a pipe for reading
     *
     * Up to 6 pipes can be open for reading at once.  Open all the
     * reading pipes, and then call startListening().
     *
     * @see openWritingPipe
     *
     * @warning Pipes 1-5 should share the first 32 bits.
     * Only the least significant byte should be unique, e.g.
     * @code
     *   openReadingPipe(1,0xF0F0F0F0AA);
     *   openReadingPipe(2,0xF0F0F0F066);
     * @endcode
     *
     * @warning Pipe 0 is also used by the writing pipe.  So if you open
     * pipe 0 for reading, and then startListening(), it will overwrite the
     * writing pipe.  Ergo, do an openWritingPipe() again before write().
     *
     * @todo Enforce the restriction that pipes 1-5 must share the top 32 bits
     *
     * @param number Which pipe# to open, 0-5.
     * @param address The 40-bit address of the pipe to open.
     */
    void openReadingPipe(uint8_t number, uint64_t address);

    /**@}*/
    /**
     * @name Optional Configurators
     *
     *  Methods you can use to get or set the configuration of the chip.
     *  None are required.  Calling begin() sets up a reasonable set of
     *  defaults.
     */
    /**@{*/
    /**
     * Set the number and delay of retries upon failed submit
     *
     * @param delay How long to wait between each retry, in multiples of 250us,
     * max is 15.  0 means 250us, 15 means 4000us.
     * @param count How many retries before giving up, max 15
     */
    void setRetries(uint8_t delay, uint8_t count);

    /**
     * Set RF communication channel
     *
     * @param channel Which RF channel to communicate on, 0-127
     */
    void setChannel(uint8_t channel);

    /**
     * Set Static Payload Size
     *
     * This implementation uses a pre-stablished fixed payload size for all
     * transmissions.  If this method is never called, the driver will always
     * transmit the maximum payload size (32 bytes), no matter how much
     * was sent to write().
     *
     * @todo Implement variable-sized payloads feature
     *
     * @param size The number of bytes in the payload
     */
    void setPayloadSize(uint8_t size);

    /**
     * Get Static Payload Size
     *
     * @see setPayloadSize()
     *
     * @return The number of bytes in the payload
     */
    uint8_t getPayloadSize(void);

    /**
     * Get Dynamic Payload Size
     *
     * For dynamic payloads, this pulls the size of the payload off
     * the chip
     *
     * @return Payload length of last-received dynamic payload
     */
    uint8_t getDynamicPayloadSize(void);

    /**
     * Enable custom payloads on the acknowledge packets
     *
     * Ack payloads are a handy way to return data back to senders without
     * manually changing the radio modes on both units.
     *
     * @see examples/pingpair_pl/pingpair_pl.pde
     */
    void enableAckPayload(void);

    /**
     * Enable dynamically-sized payloads
     *
     * This way you don't always have to send large packets just to send them
     * once in a while.  This enables dynamic payloads on ALL pipes.
     *
     * @see examples/pingpair_pl/pingpair_dyn.pde
     */
    void enableDynamicPayloads(void);

    /**
     * Determine whether the hardware is an nRF24L01+ or not.
     *
     * @return true if the hardware is nRF24L01+ (or compatible) and false
     * if its not.
     */
    bool isPVariant(void);

    /**
     * Enable or disable auto-acknowlede packets
     *
     * This is enabled by default, so it's only needed if you want to turn
     * it off for some reason.
     *
     * @param enable Whether to enable (true) or disable (false) auto-acks
     */
    void setAutoAck(bool enable);

    /**
     * Enable or disable auto-acknowlede packets on a per pipeline basis.
     *
     * AA is enabled by default, so it's only needed if you want to turn
     * it off/on for some reason on a per pipeline basis.
     *
     * @param pipe Which pipeline to modify
     * @param enable Whether to enable (true) or disable (false) auto-acks
     */
    void setAutoAck(uint8_t pipe, bool enable);

    /**
     * Set Power Amplifier (PA) level to one of four levels.
     * Relative mnemonics have been used to allow for future PA level
     * changes. According to 6.5 of the nRF24L01+ specification sheet,
     * they translate to: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm,
     * RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
     *
     * @param level Desired PA level.
     */
    void setPALevel(rf24_pa_dbm_e level);

    /**
     * Fetches the current PA level.
     *
     * @return Returns a value from the rf24_pa_dbm_e enum describing
     * the current PA setting. Please remember, all values represented
     * by the enum mnemonics are negative dBm. See setPALevel for
     * return value descriptions.
     */
    rf24_pa_dbm_e getPALevel(void);

    /**
     * Set the transmission data rate
     *
     * @warning setting RF24_250KBPS will fail for non-plus units
     *
     * @param speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
     * @return true if the change was successful
     */
    bool setDataRate(rf24_datarate_e speed);

    /**
     * Fetches the transmission data rate
     *
     * @return Returns the hardware's currently configured datarate. The value
     * is one of 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS, as defined in the
     * rf24_datarate_e enum.
     */
    rf24_datarate_e getDataRate(void);

    /**
     * Set the CRC length
     *
     * @param length RF24_CRC_8 for 8-bit or RF24_CRC_16 for 16-bit
     */
    void setCRCLength(rf24_crclength_e length);

    /**
     * Get the CRC length
     *
     * @return RF24_DISABLED if disabled or RF24_CRC_8 for 8-bit or RF24_CRC_16 for 16-bit
     */
    rf24_crclength_e getCRCLength(void);

    /**
     * Disable CRC validation
     *
     */
    void disableCRC(void);

    /**@}*/
    /**
     * @name Advanced Operation
     *
     *  Methods you can use to drive the chip in more advanced ways
     */
    /**@{*/

    /**
     * Print a giant block of debugging information to stdout
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void printDetails(void);

    /**
     * Enter low-power mode
     *
     * To return to normal power mode, either write() some data or
     * startListening, or powerUp().
     */
    void powerDown(void);

    /**
     * Leave low-power mode - making radio more responsive
     *
     * To return to low power mode, call powerDown().
     */
    void powerUp(void);

    /**
     * Test whether there are bytes available to be read
     *
     * Use this version to discover on which pipe the message
     * arrived.
     *
     * @param[out] pipe_num Which pipe has the payload available
     * @return True if there is a payload available, false if none is
     */
    bool available(uint8_t* pipe_num);

    /**
     * Non-blocking write to the open writing pipe
     *
     * Just like write(), but it returns immediately. To find out what happened
     * to the send, catch the IRQ and then call whatHappened().
     *
     * @see write()
     * @see whatHappened()
     *
     * @param buf Pointer to the data to be sent
     * @param len Number of bytes to be sent
     * @return True if the payload was delivered successfully false if not
     */
    void startWrite(const void* buf, uint8_t len);

    /**
     * Write an ack payload for the specified pipe
     *
     * The next time a message is received on @p pipe, the data in @p buf will
     * be sent back in the acknowledgement.
     *
     * @warning According to the data sheet, only three of these can be pending
     * at any time.  I have not tested this.
     *
     * @param pipe Which pipe# (typically 1-5) will get this response.
     * @param buf Pointer to data that is sent
     * @param len Length of the data to send, up to 32 bytes max.  Not affected
     * by the static payload set by setPayloadSize().
     */
    void writeAckPayload(uint8_t pipe, const void* buf, uint8_t len);

    /**
     * Determine if an ack payload was received in the most recent call to
     * write().
     *
     * Call read() to retrieve the ack payload.
     *
     * @warning Calling this function clears the internal flag which indicates
     * a payload is available.  If it returns true, you must read the packet
     * out as the very next interaction with the radio, or the results are
     * undefined.
     *
     * @return True if an ack payload is available.
     */
    bool isAckPayloadAvailable(void);

    /**
     * Call this when you get an interrupt to find out why
     *
     * Tells you what caused the interrupt, and clears the state of
     * interrupts.
     *
     * @param[out] tx_ok The send was successful (TX_DS)
     * @param[out] tx_fail The send failed, too many retries (MAX_RT)
     * @param[out] rx_ready There is a message waiting to be read (RX_DS)
     */
    void whatHappened(bool& tx_ok, bool& tx_fail, bool& rx_ready);

    /**
     * Test whether there was a carrier on the line for the
     * previous listening period.
     *
     * Useful to check for interference on the current channel.
     *
     * @return true if was carrier, false if not
     */
    bool testCarrier(void);

    /**
     * Test whether a signal (carrier or otherwise) greater than
     * or equal to -64dBm is present on the channel. Valid only
     * on nRF24L01P (+) hardware. On nRF24L01, use testCarrier().
     *
     * Useful to check for interference on the current channel and
     * channel hopping strategies.
     *
     * @return true if signal => -64dBm, false if not
     */
    bool testRPD(void);

    /**
     * Test whether this is a real radio, or a mock shim for
     * debugging.  Setting either pin to 0xff is the way to
     * indicate that this is not a real radio.
     *
     * @return true if this is a legitimate radio
     */
    bool isValid()
    { return ce_pin!=0xff && csn_pin!=0xff; }

    /**
   * This function should be called as soon as transmission is finished to
   * drop the radio back to STANDBY-I mode. If not issued, the radio will
   * remain in STANDBY-II mode which, per the data sheet, is not a recommended
   * operating mode.
   *
   * @note When transmitting data in rapid succession, it is still recommended by
   * the manufacturer to drop the radio out of TX or STANDBY-II mode if there is
   * time enough between sends for the FIFOs to empty. This is not required if auto-ack
   * is enabled.
   *
   * Relies on built-in auto retry functionality.
   *
   * @code
   * Example (Partial blocking):
   *
   *			radio.writeFast(&buf,32);
   *			radio.writeFast(&buf,32);
   *			radio.writeFast(&buf,32);  //Fills the FIFO buffers up
   *			bool ok = txStandBy();     //Returns 0 if failed. 1 if success.
   *					  				   //Blocks only until MAX_RT timeout or success. Data flushed on fail.
   * @endcode
   * @see txStandBy(unsigned long timeout)
   * @return True if transmission is successful
   *
   */
    bool txStandBy();

    /**
     * Close a pipe after it has been previously opened.
     * Can be safely called without having previously opened a pipe.
     * @param pipe Which pipe # to close, 0-5.
     */
    void closeReadingPipe( uint8_t pipe );

    /**
    * This function allows extended blocking and auto-retries per a user defined timeout
    * @code
    *	Fully Blocking Example:
    *
    *			radio.writeFast(&buf,32);
    *			radio.writeFast(&buf,32);
    *			radio.writeFast(&buf,32);   //Fills the FIFO buffers up
    *			bool ok = txStandBy(1000);  //Returns 0 if failed after 1 second of retries. 1 if success.
    *					  				    //Blocks only until user defined timeout or success. Data flushed on fail.
    * @endcode
    * @note If used from within an interrupt, the interrupt should be disabled until completion, and sei(); called to enable millis().
    * @param timeout Number of milliseconds to retry failed payloads
    * @return True if transmission is successful
    *
    */
    bool txStandBy(uint32_t timeout = TX_STANDBY_TIMEOUT, bool startTx = 0);

    /**
    * This will not block until the 3 FIFO buffers are filled with data.
    * Once the FIFOs are full, writeFast will simply wait for success or
    * timeout, and return 1 or 0 respectively. From a user perspective, just
    * keep trying to send the same data. The library will keep auto retrying
    * the current payload using the built in functionality.
    * @warning It is important to never keep the nRF24L01 in TX mode and FIFO full for more than 4ms at a time. If the auto
    * retransmit is enabled, the nRF24L01 is never in TX mode long enough to disobey this rule. Allow the FIFO
    * to clear by issuing txStandBy() or ensure appropriate time between transmissions.
    *
    * @code
    * Example (Partial blocking):
    *
    *			radio.writeFast(&buf,32);  // Writes 1 payload to the buffers
    *			txStandBy();     		   // Returns 0 if failed. 1 if success. Blocks only until MAX_RT timeout or success. Data flushed on fail.
    *
    *			radio.writeFast(&buf,32);  // Writes 1 payload to the buffers
    *			txStandBy(1000);		   // Using extended timeouts, returns 1 if success. Retries failed payloads for 1 seconds before returning 0.
    * @endcode
    *
    * @see txStandBy()
    * @see write()
    * @see writeBlocking()
    *
    * @param buf Pointer to the data to be sent
    * @param len Number of bytes to be sent
    * @return True if the payload was delivered successfully false if not
    */

    bool writeFast(const void* buf, uint8_t len);

    /**
    * WriteFast for single NOACK writes. Disables acknowledgements/autoretries for a single write.
    *
    * @note enableDynamicAck() must be called to enable this feature
    * @see enableDynamicAck()
    * @see setAutoAck()
    *
    * @param buf Pointer to the data to be sent
    * @param len Number of bytes to be sent
    * @param multicast Request ACK (0) or NOACK (1)
    */
    bool writeFast(const void* buf, uint8_t len, const bool multicast);

    /**
    * Non-blocking write to the open writing pipe used for buffered writes
    *
    * @note Optimization: This function now leaves the CE pin high, so the radio
    * will remain in TX or STANDBY-II Mode until a txStandBy() command is issued.
    * Can be used as an alternative to startWrite()
    * if writing multiple payloads at once.
    * @warning It is important to never keep the nRF24L01 in TX mode with FIFO full for more than 4ms at a time. If the auto
    * retransmit/autoAck is enabled, the nRF24L01 is never in TX mode long enough to disobey this rule. Allow the FIFO
    * to clear by issuing txStandBy() or ensure appropriate time between transmissions.
    *
    * @see write()
    * @see writeFast()
    * @see startWrite()
    * @see writeBlocking()
    *
    * For single noAck writes see:
    * @see enableDynamicAck()
    * @see setAutoAck()
    *
    * @param buf Pointer to the data to be sent
    * @param len Number of bytes to be sent
    * @param multicast Request ACK (0) or NOACK (1)
    * @return True if the payload was delivered successfully false if not
    */
    void startFastWrite( const void* buf, uint8_t len, const bool multicast, bool startTx = 1 );

    /**@}*/
};

/**
 * @example GettingStarted.pde
 *
 * This is an example which corresponds to my "Getting Started" blog post:
 * <a style="text-align:center" href="http://maniacbug.wordpress.com/2011/11/02/getting-started-rf24/">Getting Started with nRF24L01+ on Arduino</a>.
 *
 * It is an example of how to use the RF24 class.  Write this sketch to two
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting
 * with the serial monitor and sending a 'T'.  The ping node sends the current
 * time to the pong node, which responds by sending the value back.  The ping
 * node can then see how long the whole cycle took.
 */

/**
 * @example nordic_fob.pde
 *
 * This is an example of how to use the RF24 class to receive signals from the
 * Sparkfun Nordic FOB.  See http://www.sparkfun.com/products/8602 .
 * Thanks to Kirk Mower for providing test hardware.
 */

/**
 * @example led_remote.pde
 *
 * This is an example of how to use the RF24 class to control a remote
 * bank of LED's using buttons on a remote control.
 *
 * Every time the buttons change on the remote, the entire state of
 * buttons is send to the led board, which displays the state.
 */

/**
 * @example pingpair.pde
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two
 * different nodes, connect the role_pin to ground on one.  The ping node sends
 * the current time to the pong node, which responds by sending the value back.
 * The ping node can then see how long the whole cycle took.
 */

/**
 * @example pingpair_maple.pde
 *
 * This is an example of how to use the RF24 class on the Maple.  For a more
 * detailed explanation, see my blog post:
 * <a href="http://maniacbug.wordpress.com/2011/12/14/nrf24l01-running-on-maple-3/">nRF24L01+ Running on Maple</a>
 *
 * It will communicate well to an Arduino-based unit as well, so it's not for only Maple-to-Maple communication.
 *
 * Write this sketch to two different nodes,
 * connect the role_pin to ground on one.  The ping node sends the current time to the pong node,
 * which responds by sending the value back.  The ping node can then see how long the whole cycle
 * took.
 */

/**
 * @example starping.pde
 *
 * This sketch is a more complex example of using the RF24 library for Arduino.
 * Deploy this on up to six nodes.  Set one as the 'pong receiver' by tying the
 * role_pin low, and the others will be 'ping transmit' units.  The ping units
 * unit will send out the value of millis() once a second.  The pong unit will
 * respond back with a copy of the value.  Each ping unit can get that response
 * back, and determine how long the whole cycle took.
 *
 * This example requires a bit more complexity to determine which unit is which.
 * The pong receiver is identified by having its role_pin tied to ground.
 * The ping senders are further differentiated by a byte in eeprom.
 */

/**
 * @example pingpair_pl.pde
 *
 * This is an example of how to do two-way communication without changing
 * transmit/receive modes.  Here, a payload is set to the transmitter within
 * the Ack packet of each transmission.  Note that the payload is set BEFORE
 * the sender's message arrives.
 */

/**
 * @example pingpair_irq.pde
 *
 * This is an example of how to user interrupts to interact with the radio.
 * It builds on the pingpair_pl example, and uses ack payloads.
 */

/**
 * @example pingpair_sleepy.pde
 *
 * This is an example of how to use the RF24 class to create a battery-
 * efficient system.  It is just like the pingpair.pde example, but the
 * ping node powers down the radio and sleeps the MCU after every
 * ping/pong cycle.
 */

/**
 * @example scanner.pde
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

/**
 * @mainpage Driver for nRF24L01(+) 2.4GHz Wireless Transceiver
 *
 * @section Goals Design Goals
 *
 * This library is designed to be...
 * @li Maximally compliant with the intended operation of the chip
 * @li Easy for beginners to use
 * @li Consumed with a public interface that's similiar to other Arduino standard libraries
 *
 * @section News News
 *
 * NOW COMPATIBLE WITH ARDUINO 1.0 - The 'master' branch and all examples work with both Arduino 1.0 and earlier versions.
 * Please <a href="https://github.com/maniacbug/RF24/issues/new">open an issue</a> if you find any problems using it with any version of Arduino.
 *
 * NOW COMPATIBLE WITH MAPLE - RF24 has been tested with the
 * <a href="http://leaflabs.com/store/#Maple-Native">Maple Native</a>,
 * and should work with any Maple board.  See the pingpair_maple example.
 * Note that only the pingpair_maple example has been tested on Maple, although
 * the others can certainly be adapted.
 *
 * @section Useful Useful References
 *
 * Please refer to:
 *
 * @li <a href="http://maniacbug.github.com/RF24/">Documentation Main Page</a>
 * @li <a href="http://maniacbug.github.com/RF24/classRF24.html">RF24 Class Documentation</a>
 * @li <a href="https://github.com/maniacbug/RF24/">Source Code</a>
 * @li <a href="https://github.com/maniacbug/RF24/archives/master">Downloads Page</a>
 * @li <a href="http://www.nordicsemi.com/files/Product/data_sheet/nRF24L01_Product_Specification_v2_0.pdf">Chip Datasheet</a>
 *
 * This chip uses the SPI bus, plus two chip control pins.  Remember that pin 10 must still remain an output, or
 * the SPI hardware will go into 'slave' mode.
 *
 * @section More More Information
 *
 * @subpage FAQ
 *
 * @section Projects Projects
 *
 * Stuff I have built with RF24
 *
 * <img src="http://farm7.staticflickr.com/6044/6307669179_a8d19298a6_m.jpg" width="240" height="160" alt="RF24 Getting Started - Finished Product">
 *
 * <a style="text-align:center" href="http://maniacbug.wordpress.com/2011/11/02/getting-started-rf24/">Getting Started with nRF24L01+ on Arduino</a>
 *
 * <img src="http://farm8.staticflickr.com/7159/6645514331_38eb2bdeaa_m.jpg" width="240" height="160" alt="Nordic FOB and nRF24L01+">
 *
 * <a style="text-align:center" href="http://maniacbug.wordpress.com/2012/01/08/nordic-fob/">Using the Sparkfun Nordic FOB</a>
 *
 * <img src="http://farm7.staticflickr.com/6097/6224308836_b9b3b421a3_m.jpg" width="240" height="160" alt="RF Duinode V3 (2V4)">
 *
 * <a href="http://maniacbug.wordpress.com/2011/10/19/sensor-node/">Low-Power Wireless Sensor Node</a>
 *
 * <img src="http://farm8.staticflickr.com/7012/6489477865_b56edb629b_m.jpg" width="240" height="161" alt="nRF24L01+ connected to Leaf Labs Maple Native">
 *
 * <a href="http://maniacbug.wordpress.com/2011/12/14/nrf24l01-running-on-maple-3/">nRF24L01+ Running on Maple</a>
 */

#endif // __RF24_H__
// vim:ai:cin:sts=2 sw=2 ft=cpp

