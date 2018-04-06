cmake_minimum_required(VERSION 3.9)
project(nRF_Simulator)

set(CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


set(POCO_PREFIX "/usr/local/") # the directory containing "include" and "lib"
set(POCO_INCLUDE_DIR"${POCO_PREFIX}/include")
set(POCO_LIB_DIR "${POCO_PREFIX}/lib")
set(POCO_LIBS
        "${POCO_LIB_DIR}/libPocoNet.dylib"
        "${POCO_LIB_DIR}/libPocoUtil.dylib"
        "${POCO_LIB_DIR}/libPocoFoundation.dylib")

include_directories( /usr/local/include ${POCO_INCLUDE})
add_definitions(-D__linux)
set(SOURCE_FILES
        ether.cpp
        ether.h
        main.cpp
        msgframe.h
        msgframe.cpp
        nRF24bits_struct.h
        nRF24interface.cpp
        nRF24interface.h
        nRF24L01.h
        nRF24l01plus.cpp
        nRF24l01plus.h
        nRF24registers.cpp
        nRF24registers.h
        RF24_config.h
        RF24.h
        RF24.cpp
        RF24Network_config.h
        RF24Network.h
        RF24Network.cpp
        RF24MeshNode_config.h
        RF24MeshNode.h
        RF24MeshNode.cpp
        RF24MeshMaster.cpp
        RF24MeshMaster_config.h
        RF24MeshMaster.h
        MeshDHCP.cpp
        MeshDHCP.h
        MeshNode.cpp
        MeshNode.h
        MeshNodeMCU.h
        MeshNodeMCU.cpp
        MeshMasterMCU.h
        MeshMasterMCU.cpp
        )
add_executable(nRF_Simulator ${SOURCE_FILES})

target_link_libraries(nRF_Simulator ${POCO_LIBS})
