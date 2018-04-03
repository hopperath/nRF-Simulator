# nRF-Simulator
Software simulator for the NRF24l01plus modules

Building on and filling out this amazing piece of work.

1. Changed from Qt signal / slot to Poco Events
2. Finished ACK and ACK with payload functionality.
3. Converted raw pointers to shared_ptr
4. Return original STATUS register before writing. RF24 returns it on each CMD write.
5. Many other fixes
