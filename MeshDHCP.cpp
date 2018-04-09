//
// Created by Hopper II, Alan T on 3/5/18.
//


#include "RF24MeshMaster.h"
#include "MeshNode.h"

#if defined(MESH_MASTER)

//extern void delay(unsigned int millis);
extern unsigned int millis();

using namespace std;

/*****************************************************/


shared_ptr<MeshNode> MeshDHCP::findByNodeId(uint8_t nodeId)
{
    for (const auto& node : mNodes)
    {
       if (node->nodeId == nodeId)
       {
           return node;
       }
    }

    return nullptr;
}

shared_ptr<MeshNode> MeshDHCP::findByAddress(uint16_t netAddress)
{
    for (const auto& node : mNodes)
    {
        if (node->netAddress == netAddress)
        {
            return node;
        }
    }

    return nullptr;
}

void MeshDHCP::addNode(shared_ptr<MeshNode> meshNode)
{
    mNodes.push_back(meshNode);
    /*
    shared_ptr<MeshNode> newNode(meshNode);
    mNodeIdIndex[newNode->nodeId] = newNode;
    // Don't index if address is 0;
    if (newNode->netAddress != 0)
    {
        mAddressIndex[newNode->netAddress] = newNode;
    }
    */
}

void MeshDHCP::setAddress(uint8_t nodeId, uint16_t address, uint8_t state)
{
    auto node = findByNodeId(nodeId);
    if (node==nullptr)
    {
        node = shared_ptr<MeshNode>(new MeshNode(nodeId));
        addNode(node);
    }

    node->netAddress = address;
    node->state = state;

    saveDHCP();
}


void MeshDHCP::loadDHCP()
{
    ifstream infile(DHCPFILE,ifstream::binary);
    if(!infile)
    {
        return;
    }

    while (!infile.eof())
    {
        auto node = shared_ptr<MeshNode>(new MeshNode());
        infile.read((char*)node.get(),sizeof(MeshNode));

        addNode(node);
    }

    infile.close();

    printAddressList();
}


const std::vector<shared_ptr<MeshNode>>& MeshDHCP::getAddressList()
{
    return mNodes;
}

void MeshDHCP::printAddressList()
{
    int cnt=1;
    printf("  i  NodeID   Addr    State\n" );
    for(auto& item : mNodes)
    {
        MeshNode& node = *item;
        if (node.netAddress!=0)
        {
            printf("%3d:   %3d     0%-5o   %c\n",cnt++,node.nodeId,node.netAddress, node.state==MeshNode::CONFIRMED ? 'C' : 'U');
        }
    }
}

void MeshDHCP::saveDHCP()
{
    ofstream outfile (DHCPFILE,ofstream::binary | ofstream::trunc);

    printf("saveDHCP\n");
    for(auto& item : mNodes)
    {
        outfile.write( (char*)item.get(),sizeof(MeshNode));
        //printf("writingToFile %d  0%o size %d\n",addrList[0].nodeID,addrList[0].address,sizeof(addrListStruct));
    }
    printAddressList();
    outfile.close();
}

void MeshDHCP::releaseAddress(uint16_t netAddress)
{
    printf("Release node address %u\n", netAddress);
    auto node = findByAddress(netAddress);
    if (node !=nullptr)
    {
        node->netAddress = 0;
    }
    saveDHCP();
}

void MeshDHCP::releaseAddress(uint8_t nodeId)
{
    printf("Release node %u\n", nodeId);
    auto node = findByNodeId(nodeId);
    if (node !=nullptr)
    {
        node->netAddress = 0;
    }
    saveDHCP();
}

void MeshDHCP::DHCP(RF24Network& network)
{
    //TODO:: reinterpret cast?

    RF24NetworkHeader header;
    memcpy(&header,network.frame_buffer,sizeof(RF24NetworkHeader));

    uint16_t newAddress;

    // Get the unique id of the requester
    uint8_t from_id = header.reserved;
    if(from_id == 0)
    {
#ifdef MESH_DEBUG_PRINTF
        printf("%s MSH: Invalid id 0 rcvd\n",network.radio.rf24->LOGHDR);
#endif
        return;
    }

    uint16_t fwd_by = 0;
    uint8_t shiftVal = 0;
    uint8_t extraChild = 0;

    if( header.from_node == MESH_DEFAULT_ADDRESS)
    {
        //If request is coming from level 1, add an extra child to the master
        extraChild = 1;
    }
    else
    {
        fwd_by = header.from_node;
        uint16_t m = fwd_by;
        uint8_t count = 0;

        while(m) //Octal addresses convert nicely to binary in threes. Address 03 = B011  Address 033 = B011011
        {
            m >>= 3; //Find out how many digits are in the octal address
            count++;
        }
        shiftVal = count*3; //Now we know how many bits to shift when adding a child node 1-5 (B001 to B101) to any address
    }

   #ifdef MESH_DEBUG_PRINTF
    printf("%s %u MSH: Rcv addr req from_id %d from_node 0%o\n", network.radio.rf24->LOGHDR, network.millis(), from_id, header.from_node);
   #endif

    for(uint16_t i=MESH_MAX_CHILDREN+extraChild; i> 0; i--)  // For each of the possible addresses (5 max)

    {
        newAddress = fwd_by | (i << shiftVal);
        printf("%s fwd_by=0%o  newAddress=0%o\n",network.radio.rf24->LOGHDR,fwd_by,newAddress);
        if(!newAddress || newAddress == MESH_DEFAULT_ADDRESS)
        {
            //printf("dumped 0%o\n",newAddress);
            continue;
        }

        auto node = findByAddress(newAddress);

        // Not in use or request came from node already assigned so respond.
        if ( node == nullptr || node->nodeId == from_id )
        {
            header.type = NETWORK_ADDR_RESPONSE;
            header.to_node = header.from_node;
            //This is a routed request to 00
            delay(2);

            if(header.from_node == MESH_DEFAULT_ADDRESS) //Is node 01 to 05, don't retry
            {
                network.write(header,&newAddress,sizeof(newAddress),header.to_node);
            }
            else
            {
                int cnt = 0;
                while (cnt++ < 2 && !network.write(header,&newAddress,sizeof(newAddress)));
            }


            setAddress(from_id,newAddress,MeshNode::UNCONFIRMED);

#ifdef MESH_DEBUG_PRINTF
            printf("%s Sent to 0%o phys: 0%o new: 0%o id: %d\n", network.radio.rf24->LOGHDR, header.to_node,MESH_DEFAULT_ADDRESS,newAddress,header.reserved);
#endif
            break;
        }
    }

    //printAddressList();
}

#endif // MESH_MASTER
