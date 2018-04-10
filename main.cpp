#include <stdlib.h>
#include <string>
#include <chrono>
#include <iostream>
#include <vector>

#include "nRF24l01plus.h"
#include "RF24.h"
#include "ether.h"
#include "RF24Network.h"
#include "RF24MeshMaster.h"
#include "RF24MeshNode.h"
#include "MCUMeshMaster.h"
#include "MCUMeshNode.h"
#include "ThreadNames.h"

using namespace std;
using namespace chrono;

const int startNodeID = 25;
int totalNodes = 1;

bool startMaster = true;

int main(int argc, char *argv[])
{
    ThreadNames::setName("main");

    printf("Thread supported = %d\n",std::thread::hardware_concurrency());
    auto ether = shared_ptr<Ether>(new Ether());


    vector<shared_ptr<MCUMeshNode>> nodes;
    MCUMeshMaster master(ether);

    if (startMaster)
    {
        master.start();
        this_thread::yield();
    }

    for (int i=0; i<totalNodes; i++)
    {
        nodes.push_back(shared_ptr<MCUMeshNode>(new MCUMeshNode(startNodeID+i,ether)));
    }



    for (shared_ptr<MCUMeshNode> node: nodes)
    {
        node->start();
        this_thread::yield();
        this_thread::sleep_for(chrono::seconds(1));
    }

    this_thread::sleep_for(chrono::seconds(2));
    auto sixth = shared_ptr<MCUMeshNode>(new MCUMeshNode(55,ether));
    nodes.push_back(sixth);

    sixth->start();

    while(true)
    {
        fflush(stdout);
        this_thread::sleep_for(chrono::seconds(1));
    };



    printf("Sleeping");
    this_thread::yield();
    this_thread::sleep_for(chrono::seconds(15));




    /*
    //while(true);
    printf("\nExiting\n");
    master.stop();
    for (shared_ptr<MCUMeshNode> node: nodes)
    {
        node->stop();
    }
    this_thread::sleep_for(chrono::seconds(2));
     */






    /*
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

    }

     */
    return 0;
}


