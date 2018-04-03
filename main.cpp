#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"
#include "RF24Network.h"

using namespace std;
using namespace chrono;

/*
Comentting back storrry

ether should be a list of pointers to receve_frame functions...
which should copy data over to themselves and return either fail or otherwise..

each nrfClass object should have send_frame function which attempts to send a frame onto ehter,
but ether tells it if it's busy or not...

if(busy) fail;
else
{
    if(setBusy() == FALSE) fail;
    else
    {//bussy has been set, you own ethers ass now.
     coppy over the Frame
     exit and enjoy the show :D:D:D
    }
}

if ether is in one thread...

QT comments
qthread x;
x->some_public_func()
Although defined in qthread class executes the code in the main (in calling thread)
emit signal()
x->slot()
is executed in qthread
*/

void printBin(byte toPrint)
{
    int i =8;
    printf("0b");
    while(--i>=0)
    {
        printf("%d",(toPrint & 0b10000000)>>7 );
        if(i==4)
        {
            printf(" ");
        }
        toPrint<<=1;
    }
}

int main(int argc, char *argv[])
{
    Ether* ether = new Ether();

    RF24 radio(9,10,new nRF24l01plus(91,ether));
    RF24 radio2(9,10,new nRF24l01plus(92,ether));
    RF24 radio3(9,10,new nRF24l01plus(93,ether));

    RF24Network network(radio);
    network.begin(76,00);

    RF24Network network2(radio2);
    network2.begin(76,01);

    RF24Network network3(radio3);
    network3.begin(76,011);

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


    while ( !network2.available() )
    {
        // If waited longer than 200ms, indicate timeout and exit while loop                      // While nothing is received
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
        char buffer[10];
        RF24NetworkHeader hdr;
        network2.read(hdr,buffer,10);
        printf("from %u buffer2=%s\n",hdr.from_node,buffer);
    }

    return 0;
}


