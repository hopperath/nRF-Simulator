//
// Created by Hopper II, Alan T on 4/5/18.
//

#ifndef NRF_SIMULATOR_MESHMCU_H
#define NRF_SIMULATOR_MESHMCU_H

#include <thread>
#include <memory>
#include "RF24MeshNode.h"

class MeshNodeMCU
{
    private:
       std::unique_ptr<RF24> radio;
       std::unique_ptr<RF24Network> network;
       std::unique_ptr<RF24MeshNode> mesh;
       std::shared_ptr<Ether> ether;

       int nodeID = 0;
       bool running = false;

       std::thread mcu;

    public:
       MeshNodeMCU(int id, std::shared_ptr<Ether> someEther);
       void start();
       void stop();
    protected:
       void setup();
       void loop();
};

#endif //NRF_SIMULATOR_MESHMCU_H
