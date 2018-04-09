//
// Created by Hopper II, Alan T on 4/5/18.
//
#include "MCUMeshMaster.h"

using namespace std;

MCUMeshMaster::MCUMeshMaster(shared_ptr<Ether> someEther) : ether(someEther)
{
    setup();
}


void MCUMeshMaster::start()
{
    running = true;
    mcu = thread(&MCUMeshMaster::loop, this);
    mcu.detach();
}

void MCUMeshMaster::stop()
{
    running = false;
}

void MCUMeshMaster::loop()
{
    printf("%d: mcu started\n",nodeID);

    mesh->begin();

    while (running)
    {
        mesh->update();

        while (network->available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network->read(hdr,buffer,sizeof(buffer));
            printf("%d: from 0%o buffer2=%s\n",network->rf24id, hdr.from_node,buffer);
        }

        this_thread::yield();
        this_thread::sleep_for(chrono::milliseconds(2));
    }
    printf("%d: mcu stopped\n",nodeID);
}

void MCUMeshMaster::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(nodeID,ether.get()),clock));
    network = unique_ptr<RF24Network>(new RF24Network(*radio,150,clock));
    mesh = unique_ptr<RF24MeshMaster>(new RF24MeshMaster(*radio,*network,clock));
}

