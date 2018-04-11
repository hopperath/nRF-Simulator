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

bool startMaster = false;

int main(int argc, char *argv[])
{
    ThreadNames::setName("main");

    printf("Thread supported = %d\n", std::thread::hardware_concurrency());
    auto ether = shared_ptr<Ether>(new Ether());

    vector<shared_ptr<MCUMeshNode>> nodes;
    MCUMeshMaster master(ether);

    if (startMaster)
    {
        master.start();
        this_thread::yield();
    }

    for (int i = 0; i<totalNodes; i++)
    {
        nodes.push_back(shared_ptr<MCUMeshNode>(new MCUMeshNode(startNodeID + i, ether)));
    }

    for (shared_ptr<MCUMeshNode> node: nodes)
    {
        node->start();
        this_thread::yield();
        this_thread::sleep_for(chrono::seconds(1));
    }

    /*
    this_thread::sleep_for(chrono::seconds(2));
    auto sixth = shared_ptr<MCUMeshNode>(new MCUMeshNode(55,ether));
    nodes.push_back(sixth);

    sixth->start();
     */

    nodes[totalNodes-1]->triggerTX(00);

    int cnt = 0;
    while (true)
    {
        fflush(stdout);
        this_thread::sleep_for(chrono::seconds(1));

        if (cnt++==10)
        {
            master.stop();
            for (int i = 0; i<totalNodes; i++)
            {
                nodes[i]->stop();
            }
            this_thread::sleep_for(chrono::seconds(1));
        }
    }
}




