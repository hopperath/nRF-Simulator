//
// Created by Hopper II, Alan T on 4/5/18.
//

#include "MCUNetworkNode.h"
#include "ThreadNames.h"

using namespace std;

MCUNetworkNode::MCUNetworkNode(int id, uint16_t addr, shared_ptr<Ether> someEther) : ether(someEther), address(addr), node(id)
{
    setup();
}

MCUNetworkNode::~MCUNetworkNode()
{
    stop();
}


void MCUNetworkNode::start()
{
    running = true;
    mcu = thread(&MCUNetworkNode::loop, this);
    mcu.detach();
}

void MCUNetworkNode::stop()
{
    running = false;
}


void MCUNetworkNode::triggerTX(uint16_t to_node)
{
    tx = true;
    this->to_node = to_node;
}



void MCUNetworkNode::loop()
{
    ThreadNames::setName(string("mc")+to_string(node));
    printf("%s mcu started\n",radio->rf24->LOGHDR);

    network->begin(address,300);
    this_thread::yield();

    while (running)
    {
        network->update();

        while (network->available())
        {
            char buffer[32];
            RF24NetworkHeader hdr;
            network->read(hdr,buffer,sizeof(buffer));
            printf("%s RCV FROM %o buffer=%s\n",radio->rf24->LOGHDR, hdr.from_node,buffer);
        }
        this_thread::yield();
        this_thread::sleep_for(chrono::milliseconds(2));

        if (tx)
        {
            RF24NetworkHeader header;
            header.to_node = to_node;
            header.type = 90;

            string msg = string("netpayload") + to_string(node);
            network->write(header,msg.c_str(), msg.size()+1);
            tx=false;
        }
    }
    printf("%s mcu stopped\n",radio->rf24->LOGHDR);
}

void MCUNetworkNode::setup()
{
    radio = unique_ptr<RF24>(new RF24(9,10,new nRF24l01plus(node,ether.get(),clock),clock));
    network = unique_ptr<RF24Network>(new RF24Network(*radio,300,clock));
}