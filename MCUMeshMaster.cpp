//
// Created by Hopper II, Alan T on 4/5/18.
//
#include "MCUMeshMaster.h"
#include "ThreadNames.h"

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
    ThreadNames::setName(string("mc")+to_string(nodeID));
    printf("%s mcu started\n",radio->rf24->LOGHDR);

    mesh->begin();

    while (running)
    {
        mesh->update();

        while (network->available())
        {
            char buffer[32];
            RF24NetworkHeader hdr;
            network->read(hdr,buffer,sizeof(buffer));
            printf("%s from %o buffer=%s\n",radio->rf24->LOGHDR, hdr.from_node,buffer);
        }

        this_thread::yield();
        this_thread::sleep_for(chrono::milliseconds(2));
    }
    printf("%s mcu stopped\n",radio->rf24->LOGHDR);
}

void MCUMeshMaster::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(nodeID,ether.get(),clock),clock));
    network = unique_ptr<RF24Network>(new RF24Network(*radio,300,clock));
    mesh = unique_ptr<RF24MeshMaster>(new RF24MeshMaster(*radio,*network,clock));
}

