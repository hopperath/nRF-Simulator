//
// Created by Hopper II, Alan T on 4/5/18.
//

#include "MeshNodeMCU.h"
#include "RF24MeshNode.h"


using namespace std;

MeshNodeMCU::MeshNodeMCU(int id, shared_ptr<Ether> someEther) : ether(someEther), nodeID(id)
{
    setup();
}

void MeshNodeMCU::start()
{
    running = true;
    mcu = thread(&MeshNodeMCU::loop, this);
    mcu.detach();
}

void MeshNodeMCU::stop()
{
    running = false;
}


void MeshNodeMCU::loop()
{
    printf("mcu %d started\n",nodeID);

    mesh->begin();

    while (running)
    {
        mesh->update();
        this_thread::yield();
        this_thread::sleep_for(chrono::milliseconds(2));
    }
    printf("mcu %d stopped\n",nodeID);
}

void MeshNodeMCU::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(nodeID,ether.get())));
    network = unique_ptr<RF24Network>(new RF24Network(*radio));
    mesh = unique_ptr<RF24MeshNode>(new RF24MeshNode(*radio,*network));
    mesh->setNodeID(nodeID);
}