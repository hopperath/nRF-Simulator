



#include "RF24MeshNode.h"
#include "RF24MeshNode_config.h"
#include <fstream>


#if defined(__linux)
//extern void delay(unsigned int millis);
extern unsigned int millis();
#else
extern void delay(unsigned long millis);
extern unsigned long millis();
#endif

using namespace std;


RF24MeshNode::RF24MeshNode( RF24& _radio,RF24Network& _network, MCUClock& clock ) : radio(_radio),network(_network),mcuClock(clock)
{
}


/*****************************************************/

uint32_t RF24MeshNode::millis()
{
    return mcuClock.millis();
}


bool RF24MeshNode::begin(uint8_t channel, rf24_datarate_e data_rate, rf24_pa_dbm_e power, uint32_t timeout)
{
    //delay(1); // Found problems w/SPIDEV & ncurses. Without this, getch() returns a stream of garbage
    #if defined (MESH_DEBUG_SERIAL)
        Serial.println("MESH V2.0.0");
    #elif defined (MESH_DEBUG_PRINTF)
         printf("%s MESH V2.0.0\n",radio.rf24->LOGHDR);
    #endif

    radio.begin();
    radio_channel = channel;
    radio.setChannel(radio_channel);
    radio.setDataRate(data_rate);
    radio.setPALevel(power);

    network.returnSysMsgs = true;

    if(getNodeID()) //Not master node
    {
        mesh_address = MESH_DEFAULT_ADDRESS;
        if(!renewAddress(timeout))
        {
            return false;
        }
    }
#if defined(MESH_MASTER)
    else
    {
        mMeshDHCP.loadDHCP();
        mesh_address = MASTER_NODE;
        network.begin(mesh_address);
    }
#endif

    return true;
}

/*****************************************************/

uint8_t RF24MeshNode::update()
{

    uint8_t type = network.update();

    //MASTER
   #if defined(MESH_MASTER)

    if(type == NETWORK_REQ_ADDRESS)
    {
        mMeshDHCP.DHCP(network);
    }
    else if( (type == MESH_ADDR_LOOKUP || type == MESH_ID_LOOKUP))
    {
        RF24NetworkHeader* header = reinterpret_cast<RF24NetworkHeader*>(network.frame_buffer);
        header->to_node = header->from_node;

        if(type==MESH_ADDR_LOOKUP)
        {
            int16_t returnAddr = getAddress(network.frame_buffer[sizeof(RF24NetworkHeader)]);
            network.write(*header,&returnAddr,sizeof(returnAddr));
            printf("Returning addr lookup 0%o to 0%o   \n",returnAddr,header->to_node);
        }
        else
        {
            int16_t returnAddr = getNodeIdForAddress(network.frame_buffer[sizeof(RF24NetworkHeader)]);
            network.write(*header,&returnAddr,sizeof(returnAddr));
            printf("Returning id lookup 0%o to 0%o   \n",returnAddr,header->to_node);
        }
    }
    else if(type == MESH_ADDR_RELEASE )
    {
        uint16_t *fromAddr = (uint16_t*)network.frame_buffer;

        mMeshDHCP.releaseAddress(*fromAddr);
    }
    else if(type == MESH_ADDR_CONFIRM )
    {
        RF24NetworkHeader* header = reinterpret_cast<RF24NetworkHeader*>(network.frame_buffer);
        mMeshDHCP.setAddress(header->reserved,header->from_node,MeshNode::CONFIRMED);
    }

   #endif
    return type;
}



bool RF24MeshNode::write(uint16_t to_node, const void* data, uint8_t msg_type, size_t size )
{
    if(mesh_address == MESH_DEFAULT_ADDRESS)
    {
        return false;
    }
    RF24NetworkHeader header(to_node,msg_type);
    return network.write(header,data,size);
}

/*****************************************************/

bool RF24MeshNode::write(const void* data, uint8_t msg_type, size_t size, uint8_t nodeID)
{
    if(mesh_address == MESH_DEFAULT_ADDRESS)
    {
        return false;
    }

    int16_t toNode = 0;
    uint32_t lookupTimeout = millis()+ MESH_LOOKUP_TIMEOUT;
    uint32_t retryDelay = 50;

    if(nodeID)
    {

        while( (toNode=getAddress(nodeID)) < 0 )
        {
            if(millis() > lookupTimeout || toNode == -2)
            {
                return false;
            }
            retryDelay+=50;
            delay(retryDelay);
        }
    }
    return write(toNode,data,msg_type,size);
}

/*****************************************************/

void RF24MeshNode::setChannel(uint8_t _channel)
{

    radio_channel = _channel;
    radio.setChannel(radio_channel);
    radio.startListening();
}
/*****************************************************/
void RF24MeshNode::setChild(bool allow)
{
    //Prevent old versions of RF24Network from throwing an error
    //Note to remove this ""if defined"" after a few releases from 1.0.1
    #if defined FLAG_NO_POLL
    network.networkFlags = allow ? network.networkFlags & ~FLAG_NO_POLL : network.networkFlags | FLAG_NO_POLL;
    #endif
}
/*****************************************************/

bool RF24MeshNode::checkConnection()
{

    uint8_t count = 3;
    bool ok = 0;
    while(count-- && mesh_address != MESH_DEFAULT_ADDRESS)
    {
        update();
        if(radio.rxFifoFull() || (network.networkFlags & FLAG_HOLD_INCOMING))
        {
            return 1;
        }
        RF24NetworkHeader header(00,NETWORK_PING);
        ok = network.write(header,0,0);
        if(ok)
        {
            break;
        }
        delay(mesh_ping_delay);
    }
    if(!ok)
    {
        radio.stopListening();
    }
    return ok;
}

/*****************************************************/

int16_t RF24MeshNode::getAddress(uint8_t nodeID)
{

//MASTER
#if defined(MESH_MASTER)
    if(getNodeID()==MASTER_NODE)  //Master Node
    {
        auto node = mMeshDHCP.findByNodeId(nodeID);
        if (node != nullptr)
        {
            return node->netAddress;
        }
        else
        {
            return -1;
        }
    }
#endif
    if(mesh_address == MESH_DEFAULT_ADDRESS)
    {
        return -1;
    }
    if(nodeID==MASTER_NODE)
    {
        return MASTER_NODE;
    }

    //TODO:: looping on network.update could drop MESH MSG
    //       move to event pattern
    RF24NetworkHeader header( 00, MESH_ADDR_LOOKUP );
    if(network.write(header,&nodeID,sizeof(nodeID)+1) )
    {
        uint32_t timer=millis();
        uint32_t timeout = mesh_get_addr_timeout;
        while(network.update() != MESH_ADDR_LOOKUP)
        {
            YIELD();
            if(millis()-timer > timeout)
            {
                return -1;
            }
        }
    }
    else
    {
        return -1;
    }
    int16_t address = 0;
    memcpy(&address,network.frame_buffer+sizeof(RF24NetworkHeader),sizeof(address));
    return address >= 0 ? address : -2;
}

uint8_t RF24MeshNode::getNodeID()
{
    return _nodeID;
}

int16_t RF24MeshNode::getNodeIdForAddress(uint16_t address)
{

    if(address == 0)
    {
        return 0;
    }
    else if (address == mesh_address) //This node
    {
        return mesh_address;
    }

    if(mesh_address)
    {
        if(mesh_address == MESH_DEFAULT_ADDRESS)
        {
            return -1;
        }


        RF24NetworkHeader header( 00, MESH_ID_LOOKUP );
        if(network.write(header,&address,sizeof(address)) )
        {
            uint32_t timer=millis(), timeout = 500;
            // This ok for a node, mesh.update just calls network.update
            while(network.update() != MESH_ID_LOOKUP)
            {
                if(millis()-timer > timeout)
                {
                    return -1;
                }
            }
            int16_t ID;
            memcpy(&ID,&network.frame_buffer[sizeof(RF24NetworkHeader)],sizeof(ID));
            return ID;
        }
    }
#if defined(MESH_MASTER)
    else  //Master Node
    {
        auto node = mMeshDHCP.findByAddress(address);
        if (node !=nullptr)
        {
            return node->nodeId;
        }
    }
#endif

    return -1;
}
/*****************************************************/

bool RF24MeshNode::releaseAddress()
{

    if(mesh_address == MESH_DEFAULT_ADDRESS)
    {
        return 0;
    }

    RF24NetworkHeader header(00,MESH_ADDR_RELEASE);
    if(network.write(header,0,0))
    {
        network.begin(MESH_DEFAULT_ADDRESS);
        mesh_address=MESH_DEFAULT_ADDRESS;
        return 1;
    }
    return 0;
}

/*****************************************************/

uint16_t RF24MeshNode::renewAddress(uint32_t timeout)
{

    if(radio.available())
    {
        return 0;
    }
    uint8_t reqCounter = 0;
    uint8_t totalReqs = 0;
    radio.stopListening();

    network.networkFlags |= FLAG_BYPASS_HOLDS;
    delay(10);

    network.begin(MESH_DEFAULT_ADDRESS);
    mesh_address = MESH_DEFAULT_ADDRESS;

    uint32_t start = millis();
    while(!requestAddress(reqCounter))
    {
        if(millis()-start > timeout)
        {
            return 0;
        }
        delay(50 + ( (totalReqs+1)*(reqCounter+1)) * 2);
        reqCounter = ++reqCounter%4;
        totalReqs = ++totalReqs%10;
    }
    network.networkFlags &= ~FLAG_BYPASS_HOLDS;
    return mesh_address;
}

/*****************************************************/



bool RF24MeshNode::requestAddress(uint8_t level)
{

    RF24NetworkHeader header( MULTICAST_ADDRESS_NODE, NETWORK_POLL );
    //Find another radio, starting with level 0 multicast
    #if defined (MESH_DEBUG_SERIAL)
    Serial.print( millis() );
    Serial.print(F(" MSH: Poll Level "));
    Serial.println(level);
    #endif
    //radio.dumpRegisters("before multicast\n");
    network.multicast(header,0,0,level);
    //radio.dumpRegisters("after multicast\n");

    uint32_t timr = millis();
    uint16_t contactNode[MESH_MAXPOLLS];
    uint8_t pollCount=0;

    //TODO: SIM ONLY
    int yield = 0;

    while(true)
    {
        #if defined (MESH_DEBUG_SERIAL) || defined (MESH_DEBUG_PRINTF)
        bool goodSignal = radio.testRPD();
        #endif
        // This ok for a node, mesh.update just calls network.update
        if(network.update() == NETWORK_POLL)
        {
            memcpy(&contactNode[pollCount],&network.frame_buffer[0],sizeof(uint16_t));
            ++pollCount;

            #if defined (MESH_DEBUG_SERIAL) || defined (MESH_DEBUG_PRINTF)
            if(goodSignal)
            {
                // This response was better than -64dBm
                #if defined (MESH_DEBUG_SERIAL)
                Serial.print( millis() ); Serial.println(F(" MSH: Poll > -64dbm "));
                #elif defined (MESH_DEBUG_PRINTF)
                printf( "%s %u MSH: Poll > -64dbm\n", radio.rf24->LOGHDR,millis() );
                #endif
            }
            else
            {
                #if defined (MESH_DEBUG_SERIAL)
                Serial.print( millis() ); Serial.println(F(" MSH: Poll < -64dbm "));
                #elif defined (MESH_DEBUG_PRINTF)
                printf( "%s %u MSH: Poll < -64dbm\n", radio.rf24->LOGHDR,millis() );
                #endif
            }
            #endif
        }

        if(millis() - timr > mesh_poll_timeout || pollCount >=  MESH_MAXPOLLS )
        {
            if(!pollCount)
            {
              #if defined (MESH_DEBUG_SERIAL)
                Serial.print( millis() ); Serial.print(F(" MSH: No poll from level ")); Serial.println(level);
              #elif defined (MESH_DEBUG_PRINTF)
                printf( "%s %u MSH: No poll from level %d\n", radio.rf24->LOGHDR,millis(), level);
              #endif
                return 0;
            }
            else
            {

              #if defined (MESH_DEBUG_SERIAL)
                Serial.print( millis() );
                Serial.print(F(" MSH: Poll OK Level "));
                Serial.println(level);
              #elif defined (MESH_DEBUG_PRINTF)
                printf( "%s %u MSH: Poll OK %u responses\n", radio.rf24->LOGHDR,millis() , pollCount);
              #endif
                break;
            }
        }

        YIELDAT(yield);

    }


    #ifdef MESH_DEBUG_SERIAL
    Serial.print( millis() );
    Serial.print(F(" MSH: Got poll from level "));
    Serial.print(level);
    Serial.print(F(" count "));
    Serial.println(pollCount);
    #elif defined MESH_DEBUG_PRINTF
    printf("%s %u MSH: Got poll from level %d count %d\n",radio.rf24->LOGHDR,millis(),level,pollCount);
    #endif

    uint8_t type=0;
    for(uint8_t i=0; i<pollCount; i++)
    {
        // Request an address via the contact node
        header.type = NETWORK_REQ_ADDRESS;
        header.reserved = getNodeID();
        header.to_node = contactNode[i];

        // Do a direct write (no ack) to the contact node. Include the nodeId and address.
        network.write(header,0,0,contactNode[i]);
    #ifdef MESH_DEBUG_SERIAL
        Serial.print( millis() ); Serial.print(F(" MSH: Req addr from ")); Serial.println(contactNode[i],OCT);
    #elif defined MESH_DEBUG_PRINTF
        printf("%s %u MSH: Request address from: 0%o\n",radio.rf24->LOGHDR,millis(),contactNode[i]);
    #endif

        timr = millis();

        while(millis()-timr < network_addr_response_timeout)
        {
            YIELD();
            if( (type = network.update()) == NETWORK_ADDR_RESPONSE)
            {
                i=pollCount;
                break;
            }
        }
        delay(5);
    }

    if(type != NETWORK_ADDR_RESPONSE)
    {
        printf("%s No network addr response\n", radio.rf24->LOGHDR);
        return 0;
    }

    //Serial.print("response took");
    //Serial.println(millis()-timr);
    #ifdef MESH_DEBUG_SERIAL
    uint8_t mask = 7;
    char addrs[5] = "    ";
    uint8_t count=3;
    uint16_t newAddr;
    #endif

    uint16_t newAddress=0;
    memcpy(&newAddress,network.frame_buffer+sizeof(RF24NetworkHeader),sizeof(newAddress));

    auto fbHeader = reinterpret_cast<RF24NetworkHeader*>(network.frame_buffer);

    // Check response to see if address request is for this node
    if(!newAddress || fbHeader->reserved != getNodeID() )
    {
        #ifdef MESH_DEBUG_SERIAL
        Serial.print(millis());
        Serial.print(F(" MSH: Attempt Failed. Response for"));
        Serial.print(fbHeader->reserved);
        Serial.print("  This NodeID: "); Serial.println(getNodeID());
        #elif defined MESH_DEBUG_PRINTF
        printf("%s %u Response discarded, wrong node 0%o from node 0%o sending node 0%o id %d\n",radio.rf24->LOGHDR,millis(),newAddress,header.from_node,MESH_DEFAULT_ADDRESS,network.frame_buffer[7]);
        #endif
        return 0;
    }
    #ifdef MESH_DEBUG_SERIAL
    Serial.print( millis() ); Serial.print(F(" Set address: "));
    newAddr = newAddress;
    while(newAddr)
    {
        addrs[count] = (newAddr & mask)+48; //get the individual Octal numbers, specified in chunks of 3 bits, convert to ASCII by adding 48
        newAddr >>= 3;
        count--;
    }
    Serial.println(addrs);
    #elif defined (MESH_DEBUG_PRINTF)
    printf("%s Set address 0%o rcvd 0%o\n",radio.rf24->LOGHDR,mesh_address,newAddress);
    #endif
    mesh_address = newAddress;

    radio.stopListening();
    delay(10);
    network.begin(mesh_address);
    header.to_node = 00;
    header.type = MESH_ADDR_CONFIRM;

    uint8_t registerAddrCount = 0;

    while( !network.write(header,0,0) )
    {
        if(registerAddrCount++ >= 6 )
        {
            network.begin(MESH_DEFAULT_ADDRESS);
            mesh_address = MESH_DEFAULT_ADDRESS;
            return false;
        }
        delay(3);
    }

    return true;
}

/*****************************************************/
/*
   bool RF24MeshNode::waitForAvailable(uint32_t timeout){

    unsigned long timer = millis();
    while(millis()-timer < timeout){
      network.update();
      if(network.available()){ return 1; }
    }
    if(network.available()){ return 1; }
    else{  return 0; }
   }
 */
/*****************************************************/

void RF24MeshNode::setNodeID(uint8_t nodeID)
{
    _nodeID = nodeID;
}

/*****************************************************/
