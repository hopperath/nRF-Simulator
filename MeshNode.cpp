//
// Created by Hopper II, Alan T on 3/6/18.
//
#include "MeshNode.h"


#if defined(MESH_MASTER)

MeshNode::MeshNode() {}

MeshNode::MeshNode(uint8_t nodeId)
{
    this->nodeId = nodeId;
}

MeshNode::MeshNode(uint8_t nodeId, uint16_t netAddress)
{
    this->nodeId = nodeId;
    this->netAddress = netAddress;
}

bool MeshNode::operator==(const MeshNode& obj2) const
{
    return (this->nodeId == obj2.nodeId);
}

#endif //MESH_MASTER
