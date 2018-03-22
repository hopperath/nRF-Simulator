#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"

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
        if(i==4)printf(" ");
        toPrint<<=1;
    }
}
int main(int argc, char *argv[])
{
    Ether* ether = new Ether();
    nRF24l01plus* test = new nRF24l01plus(ether);
    //nRF24l01plus* test2 = new nRF24l01plus(0,ether);




    RF24* radio = new RF24(9,10,test);
    //RF24* radio2 = new RF24(9,10,test2);

    radio->begin();
    radio->setRetries(15,15);
    radio->setPayloadSize(8);
    radio->openWritingPipe(0xF0F0F0F0E1);
    radio->openReadingPipe(1, 0xF0F0F0F0D2);

    /*
    radio2->begin();
    radio2->setRetries(15,15);
    radio2->setPayloadSize(8);
    radio2->openWritingPipe(0xF0F0F0F0D2);
    radio2->openReadingPipe(1, 0xF0F0F0F0E1);
    */

    //radio->startListening();
    radio->stopListening();
    getchar();
    //radio->printDetails();
    printf("writing\n");
    char payload[] = "test";
    radio->write("test",sizeof(payload));

    /*
    radio2->stopListening();
    radio2->printDetails();
    printf("writing");
    radio2->write("test",5);
    */
    
    return 0;
}


