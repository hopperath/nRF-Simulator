//
// Created by Hopper II, Alan T on 4/5/18.
//

#include "MCUMeshNode.h"
#include "RF24MeshNode.h"


using namespace std;

MCUMeshNode::MCUMeshNode(int id, shared_ptr<Ether> someEther) : ether(someEther), nodeID(id)
{
    setup();
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
    printf("%d: mcu started\n",nodeID);

    mesh->begin();
    this_thread::yield();

    bool tx = true;

    while (running)
    {
        mesh->update();

        while (network->available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network->read(hdr,buffer,sizeof(buffer));
            printf("%d: from %o buffer2=%s\n",network->rf24id, hdr.from_node,buffer);
        }
        this_thread::yield();
        this_thread::sleep_for(chrono::milliseconds(2));

        //if (tx && mesh->mesh_address!=MESH_DEFAULT_ADDRESS)
        if (tx)
        {
            RF24NetworkHeader header;
            header.to_node = 00;
            header.type = 75;
            char msg[15] = "sendTo00";
            network->write(header,msg,sizeof(msg));
            tx=false;
        }
    }
    printf("%d: mcu stopped\n",nodeID);
}

void MCUMeshNode::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(nodeID,ether.get(),clock),clock));
    network = unique_ptr<RF24Network>(new RF24Network(*radio,150,clock));
    mesh = unique_ptr<RF24MeshNode>(new RF24MeshNode(*radio,*network,clock));
    mesh->setNodeID(nodeID);
}