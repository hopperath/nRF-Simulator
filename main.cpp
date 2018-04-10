#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"
#include "RF24Network.h"
#include "ThreadNames.h"

using namespace std;
using namespace chrono;


int main(int argc, char *argv[])
{
    MCUClock clock;
    Ether* ether = new Ether();
    ThreadNames::setName("main");

    RF24 radio(9,10,new nRF24l01plus(90,ether,clock),clock);
    RF24 radio2(9,10,new nRF24l01plus(91,ether,clock),clock);
    RF24 radio3(9,10,new nRF24l01plus(92,ether,clock),clock);

    RF24Network network(radio,200,clock);
    radio.begin();
    network.begin(76,00,300);

    RF24Network network2(radio2,200,clock);
    radio2.begin();
    network2.begin(76,01,300);

    RF24Network network3(radio3,20000,clock);
    radio3.begin();
    network3.begin(76,011,300);



    int cnt = 0;
    while (true)
    {
        if (cnt==100 || cnt==0)
        {
            RF24NetworkHeader header;
            header.from_node = 011;
            header.to_node = 00;
            header.type = 90;

            string msg = string("netpayload") + to_string(cnt);
            network3.write(header,msg.c_str(), msg.size()+1);
        }

        cnt++;

        if (cnt==200)
        {
            fflush(stdout);
            break;
        }

        network3.update();
        network2.update();
        network.update();

        if (network.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer2=%s\n", hdr.from_node, buffer);
        }

        if (network2.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network2.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer2=%s\n", hdr.from_node, buffer);
        }

        if (network3.available())
        {
            char buffer[20];
            RF24NetworkHeader hdr;
            network3.read(hdr, buffer, sizeof(buffer));
            printf("from %o buffer2=%s\n", hdr.from_node, buffer);
        }



        this_thread::sleep_for(milliseconds(1));
    }
    return 0;
}


