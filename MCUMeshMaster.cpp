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
MCUMeshMaster::~MCUMeshMaster()
{
    stop();
}

void MCUMeshMaster::triggerTX(uint16_t to_node)
{
    tx = true;
    this->to_node = to_node;
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

        if (tx)
        {
            RF24NetworkHeader header;
            header.to_node = to_node;
            header.type = 90;

            string msg = string("netpayload") + to_string(nodeID);
            network->write(header,msg.c_str(), msg.size()+1);
            tx=false;
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

