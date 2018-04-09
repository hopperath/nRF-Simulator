#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"

using namespace std;
using namespace chrono;

int main(int argc, char *argv[])
{
    Ether* ether = new Ether();
    MCUClock clock;


    RF24* radio = new RF24(9,10,new nRF24l01plus(91,ether,clock),clock);
    //RF24* radio2 = new RF24(9,10,new nRF24l01plus(92,ether,clock),clock);
    //RF24* radio3 = new RF24(9,10,new nRF24l01plus(93,ether));

    radio->begin();
    radio->setRetries(5,5);
    radio->setPayloadSize(32);
    //radio->enableAckPayload();
    radio->openWritingPipe(0xF0F0F0F0A1);
    radio->openReadingPipe(1, 0xF0F0F0F010);
    radio->stopListening();
    radio->enableDynamicAck();
    radio->setAutoAck(0,true);

    //radio2->begin();
    //radio2->setRetries(5,1);
    //radio2->setPayloadSize(32);
    //radio2->enableAckPayload();
    //radio2->printDetails();
    //radio2->openWritingPipe(0xF0F0F0F010);
    //radio2->openReadingPipe(1, 0xF0F0F0F0A1);
    //radio2->setAutoAck(true);
    //radio2->startListening();


    /*
    radio3->begin();
    radio3->setRetries(5,1);
    radio3->setPayloadSize(32);
    radio3->enableAckPayload();
    radio3->openWritingPipe(0xF0F0F0F010);
    radio3->openReadingPipe(1, 0xF0F0F0F0B1);
     */


    //char payload2[] = "ackPayload2";
    //radio2->writeAckPayload(1,payload2, sizeof(payload2));
    //radio2->startListening();

    //char payload3[] = "ackPayload3";
    //radio3->writeAckPayload(1,payload3, sizeof(payload3));
    //radio3->startListening();

    char payload[] = "payload2";
    bool ok = radio->write(payload,sizeof(payload));
    printf("write2 ok=%u\n",ok);

    //radio->openWritingPipe(0xF0F0F0F0B1);
    //char pay3[] = "payload3";
    //ok = radio->write(pay3,sizeof(pay3));
    //printf("write3 ok=%u\n",ok);

    fflush(stdout);



    while (true)
        this_thread::sleep_for(seconds(5));
    /*
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
        printf("Radio2 Timeout waiting for pkt\n");
    }
    else
    {
        char buffer[10];
        radio2->read(buffer,10);
        printf("buffer2=%s\n",buffer);
    }
     */

    /*
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
        printf("Radio3 Timeout waiting for pkt\n");
    }
    else
    {
        char buffer[10];
        radio3->read(buffer,10);
        printf("buffer3=%s\n",buffer);
    }

    if (radio->isAckPayloadAvailable())
    {
        char buffer2[15];
        while (radio->available())
        {
            radio->read(buffer2, sizeof(buffer2));
            printf("ack buffer=%s\n", buffer2);
        }
    }
     */

    /*
    radio2->stopListening();
    radio2->printDetails();
    printf("writing");
    radio2->write("test",5);
    */
    
    return 0;
}


