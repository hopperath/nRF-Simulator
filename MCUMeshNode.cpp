//
// Created by Hopper II, Alan T on 4/5/18.
//

#include "MCUMeshNode.h"
#include "RF24MeshNode.h"
#include "ThreadNames.h"

using namespace std;

MCUMeshNode::MCUMeshNode(int id, shared_ptr<Ether> someEther) : ether(someEther), nodeID(id)
{
    setup();
}

MCUMeshNode::~MCUMeshNode()
{
    stop();
}

void MCUMeshNode::triggerTX(uint16_t to_node)
{
    tx = true;
    this->to_node = to_node;
}

void MCUMeshNode::start()
{
    running = true;
    mcu = thread(&MCUMeshNode::loop, this);
    mcu.detach();
}

void MCUMeshNode::stop()
{
    running = false;
}


void MCUMeshNode::loop()
{
    ThreadNames::setName(string("mc")+to_string(nodeID));
    printf("%s mcu started\n",radio->rf24->LOGHDR);

    mesh->begin();
    this_thread::yield();

    while (running)
    {
        mesh->update();

        while (network->available())
        {
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

void MCUMeshNode::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(nodeID,ether.get(),clock),clock));
    network = unique_ptr<RF24Network>(new RF24Network(*radio,300,clock));
    mesh = unique_ptr<RF24MeshNode>(new RF24MeshNode(*radio,*network,clock));
    mesh->setNodeID(nodeID);
    mesh->mesh_ping_delay = 150;
    mesh->mesh_get_addr_timeout = 200;
    mesh->mesh_poll_timeout = 100;
    mesh->network_addr_response_timeout = 250;
}
