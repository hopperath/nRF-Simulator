#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"

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

    RF24* radio = new RF24(9,10,new nRF24l01plus(91,ether));
    RF24* radio2 = new RF24(9,10,new nRF24l01plus(92,ether));
    RF24* radio3 = new RF24(9,10,new nRF24l01plus(93,ether));

    radio->begin();
    radio->setRetries(5,1);
    radio->setPayloadSize(32);
    radio->enableAckPayload();
    radio->openWritingPipe(0xF0F0F0F0A1);
    radio->openReadingPipe(1, 0xF0F0F0F010);
    radio->stopListening();

    radio2->begin();
    radio2->setRetries(5,1);
    radio2->setPayloadSize(32);
    radio2->enableAckPayload();
    //radio2->printDetails();
    radio2->openWritingPipe(0xF0F0F0F010);
    radio2->openReadingPipe(1, 0xF0F0F0F0A1);


    radio3->begin();
    radio3->setRetries(5,1);
    radio3->setPayloadSize(32);
    radio3->enableAckPayload();
    radio3->openWritingPipe(0xF0F0F0F010);
    radio3->openReadingPipe(1, 0xF0F0F0F0B1);


    char payload2[] = "ackPayload2";
    radio2->writeAckPayload(1,payload2, sizeof(payload2));
    radio2->startListening();

    char payload3[] = "ackPayload3";
    radio3->writeAckPayload(1,payload3, sizeof(payload3));
    radio3->startListening();

    char payload[] = "payload2";
    bool ok = radio->write(payload,sizeof(payload));
    printf("write2 ok=%u\n",ok);

    radio->openWritingPipe(0xF0F0F0F0B1);
    char pay3[] = "payload3";
    ok = radio->write(pay3,sizeof(pay3));
    printf("write3 ok=%u\n",ok);

    auto start = steady_clock::now();

    auto started_waiting_at = steady_clock::now();
    milliseconds RxTimeout(200);
    bool timeout = false;


    while ( !radio2->available() )
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
        radio2->read(buffer,10);
        printf("buffer2=%s\n",buffer);
    }

    started_waiting_at = steady_clock::now();

    while ( !radio3->available() )
    {
        // If waited longer than 200ms, indicate timeout and exit while loop                      // While nothing is received
        if (steady_clock::now() - started_waiting_at > RxTimeout)
        {
            timeout=true;
            break;
        }
    }

    if (timeout)
    {
        printf("Timeout!");
    }
    else
    {
        char buffer[10];
        radio3->read(buffer,10);
        printf("buffer3=%s\n",buffer);
    }

    if (radio->isAckPayloadAvailable())
    {
        char buffer[10];
        while (radio->read(buffer, sizeof(buffer)))
        {
            printf("ack buffer=%s\n", buffer);
        }
    }
    /*
    radio2->stopListening();
    radio2->printDetails();
    printf("writing");
    radio2->write("test",5);
    */
    
    return 0;
}


