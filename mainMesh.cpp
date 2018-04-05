#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"
#include "RF24Network.h"
#include "RF24MeshMaster.h"
#include "RF24MeshNode.h"

using namespace std;
using namespace chrono;


int main(int argc, char *argv[])
{
    Ether* ether = new Ether();

    //RF24 radio3(9,10,new nRF24l01plus(93,ether));

    RF24 radio(9,10,new nRF24l01plus(91,ether));
    RF24Network network(radio);
    RF24MeshMaster node00(radio,network);
    node00.begin();


    RF24 radio2(9,10,new nRF24l01plus(92,ether));
    RF24Network network2(radio2);
    RF24MeshNode node02(radio2,network2);
    node02.setNodeID(25);
    node02.begin();

    /*
    RF24Network network2(radio2);
    radio2.begin();
    network2.begin(76,01);

    //RF24Network network3(radio3);
    //network3.begin(76,011);

    RF24NetworkHeader header;
    header.from_node = 00;
    header.to_node = 01;
    header.type = 90;

    char msg[]="netpayload01";
    network.write(header,msg,sizeof(msg));

    auto start = steady_clock::now();

    auto started_waiting_at = steady_clock::now();
    milliseconds RxTimeout(200);
    bool timeout = false;

    network2.update();

    while ( !network2.available() )
    {
        // If waited longer than 200ms, indicate timeout and exit while loop
        // While nothing is received
        if (steady_clock::now() - started_waiting_at > RxTimeout)
        {
            timeout=true;
            break;
        }
    }
    auto end = steady_clock::now();

    cout << "duration=" << duration_cast<milliseconds>(end-start).count() << endl;

    if (timeout)
    {
        printf("Timeout!");
    }
    else
    {
        char buffer[20];
        RF24NetworkHeader hdr;
        network2.read(hdr,buffer,sizeof(buffer));
        printf("from %o buffer2=%s\n",hdr.from_node,buffer);
    }

     */
    return 0;
}


