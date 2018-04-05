//
// Created by Hopper II, Alan T on 3/5/18.
//

#ifndef PARK_GATEWAY_MESHNODE_H
#define PARK_GATEWAY_MESHNODE_H
#include "RF24MeshMaster_config.h"

#if defined(MESH_MASTER)
#include <cstdint>

class MeshNode
{
    public:
        uint8_t nodeId;    /**< NodeIDs and addresses are stored using this structure */
        uint16_t netAddress;  /**< NodeIDs and addresses are stored using this structure */
        uint8_t state;

    static const uint8_t UNCONFIRMED = 0;
    static const uint8_t CONFIRMED   = 1;

    MeshNode();
    MeshNode(uint8_t nodeId);
    MeshNode(uint8_t nodeId, uint16_t netAddress);
    bool operator==(const MeshNode& obj2) const;
};

#endif //MESH_MASTER

#endif //PARK_GATEWAY_MESHNODE_H
