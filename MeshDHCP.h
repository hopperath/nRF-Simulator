//
// Created by Hopper II, Alan T on 3/5/18.
//

#ifndef PARK_GATEWAY_MESHDHCP_H
#define PARK_GATEWAY_MESHDHCP_H

#include "RF24Mesh_config.h"

#if defined(MESH_MASTER)

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <ios>
#include <fstream>
#include "MeshNode.h"

class MeshDHCP
{

    const uint8_t UNCONFIRMED = 0;
    const uint8_t CONFIRMED   = 1;
    const std::string DHCPFILE = "dhcplist.dat";

    std::vector<std::shared_ptr<MeshNode>> mNodes;

    void addNode(std::shared_ptr<MeshNode> meshNode);




    //std::map<uint8_t,std::shared_ptr<MeshNode>> mNodeIdIndex;
    //std::map<int16_t,std::shared_ptr<MeshNode>> mAddressIndex;

    public:
        std::shared_ptr<MeshNode> findByNodeId(uint8_t nodeId);
        std::shared_ptr<MeshNode> findByAddress(uint16_t netAddress);
        void saveDHCP();
        void loadDHCP();
        const std::vector<std::shared_ptr<MeshNode>>& getAddressList();
        void printAddressList();
        void releaseAddress(uint8_t nodeId);
        void releaseAddress(uint16_t netAddress);
        void DHCP(RF24Network& network);

       /**
        * Set/change a nodeID/RF24Network Address pair manually on the master node.
        * @param nodeID The nodeID to assign
        * @param address The octal RF24Network address to assign
        * @return If the nodeID exists in the list,
        */
        void setAddress(uint8_t nodeID, uint16_t address, uint8_t state);
};

#endif //MESH_MASTER

#endif //PARK_GATEWAY_MESHDHCP_H
