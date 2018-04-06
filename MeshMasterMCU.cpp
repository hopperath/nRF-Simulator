//
// Created by Hopper II, Alan T on 4/5/18.
//
#include "MeshMasterMCU.h"

using namespace std;

MeshMasterMCU::MeshMasterMCU(shared_ptr<Ether> someEther) : ether(someEther)
{
    setup();
}


void MeshMasterMCU::start()
{
    running = true;
    mcu = thread(&MeshMasterMCU::loop, this);
    mcu.detach();
}

void MeshMasterMCU::stop()
{
    running = false;
}

void MeshMasterMCU::loop()
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

void MeshMasterMCU::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(nodeID,ether.get())));
    network = unique_ptr<RF24Network>(new RF24Network(*radio));
    mesh = unique_ptr<RF24MeshMaster>(new RF24MeshMaster(*radio,*network));
}

