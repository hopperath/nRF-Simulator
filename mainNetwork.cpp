#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"
#include "RF24Network.h"
#include "ThreadNames.h"
#include "MCUNetworkNode.h"

using namespace std;
using namespace chrono;


int main(int argc, char *argv[])
{
    MCUClock clock;
    auto ether = shared_ptr<Ether>(new Ether());
    ThreadNames::setName("main");

    MCUNetworkNode node00(0,00,ether);
    MCUNetworkNode node01(1,01,ether);
    MCUNetworkNode node011(2,011,ether);
    MCUNetworkNode node0111(3,0111,ether);
    MCUNetworkNode node01111(4,01111,ether);


    node00.start();
    node01.start();
    node011.start();
    node0111.start();
    node01111.start();

    node01111.triggerTX(00);


    int cnt = 0;
    while (true)
    {
        /*
        if (cnt==100 || cnt==0)
        {
            RF24NetworkHeader header;
            header.to_node = 00;
            header.type = 90;

            string msg = string("netpayload") + to_string(cnt);
            network4.write(header,msg.c_str(), msg.size()+1);
        }


        if (cnt==200)
        {
            fflush(stdout);
            break;
        }

        cnt++;

        network4.update();
        network3.update();
        network2.update();
        network1.update();
        network0.update();

        if (network0.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network0.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer=%s\n", hdr.from_node, buffer);
        }

        if (network1.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network1.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer=%s\n", hdr.from_node, buffer);
        }

        if (network2.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network2.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer=%s\n", hdr.from_node, buffer);
        }

        if (network4.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network4.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer=%s\n", hdr.from_node, buffer);
        }
         */



        this_thread::sleep_for(seconds(1));
        fflush(stdout);
        if (cnt++ > 2)
        {
            node00.stop();
            node01.stop();
            node011.stop();
            node0111.stop();
            node01111.stop();
            this_thread::sleep_for(seconds(1));
            break;
        }
    }
    return 0;
}


