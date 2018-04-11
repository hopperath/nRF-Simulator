//
// Created by Hopper II, Alan T on 4/5/18.
//

#ifndef NRF_SIMULATOR_NETWORKMCU_H
#define NRF_SIMULATOR_NETWORKMCU_H

#include <thread>
#include <memory>
#include "RF24MeshNode.h"
#include "MCUClock.h"

class MCUNetworkNode
{
    public:
       MCUClock clock;

    private:
       std::unique_ptr<RF24> radio;
       std::unique_ptr<RF24Network> network;
       std::shared_ptr<Ether> ether;

       int node = 0;
       uint16_t address;
       uint16_t to_node;
       bool tx = false;
       bool running = false;

       std::thread mcu;

    public:
       MCUNetworkNode(int id, uint16_t addr, std::shared_ptr<Ether> someEther);
       ~MCUNetworkNode();
       void start();
       void stop();
       void triggerTX(uint16_t to_node);
    protected:
       void setup();
       void loop();
};

#endif //NRF_SIMULATOR_MESHMCU_H
