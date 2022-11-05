

import inet.common.INETDefs;
import inet.common.packet.ChunkBuffer;
import inet.common.packet.ChunkQueue;
import inet.common.packet.Packet;
import inet.networklayer.common.L3Address;

cplusplus {{
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"
#include "RdpReceiveQueue.h"
#include "rdp_common/RdpHeader.h"
}}

namespace inet::rdp;

class RdpAlgorithm extends cObject { @existingClass; }

struct RdpStateVariables
{
    @existingClass;
    @descriptor(readonly);
    
    bool active;
    
    unsigned int request_id;
    unsigned int internal_request_id;
    uint32 IW;
    uint32 cwnd;
    uint32 ssthresh;
    uint32 receivedPacketsInWindow;
    uint32 sentPullsInWindow;
    uint32 additiveIncreasePackets;
    uint32 slowStartPacketsToSend;
    uint32 outOfWindowPackets;
    bool connFinished;
    bool slowStartState;
    bool congestionInWindow;
    bool waitToStart;
    int delayedNackNo;
    uint32 numPacketsToGet;
    uint32 numPacketsToSend;

    unsigned int numRcvdPkt;
    unsigned int numRcvTrimmedHeader;


    uint32 numberReceivedPackets;
    uint32 numberSentPackets;

    bool connNotAddedYet;
    bool isfinalReceivedPrintedOut;
};

class RdpSendQueue extends cObject
{
    @existingClass;
    @descriptor(readonly);
    cPacketQueue dataToSendQueue;      // dataBuffer
}

class RdpReceiveQueue extends cObject
{
    @existingClass;
    @descriptor(readonly);
    cPacketQueue receiveBuffer;      // dataBuffer
}

class RdpConnection extends cObject
{
    @existingClass;
    @descriptor(readonly);

    int socketId;    // identifies connection within the app
    int listeningSocketId;    // identifies listening connection within the app

    // socket pair
    L3Address localAddr;
    L3Address remoteAddr;
    int localPort;
    int remotePort;

    RdpStateVariables *state;

    // NDP queues
    RdpSendQueue *sendQueue;
	RdpReceiveQueue *receiveQueue;
	
    RdpAlgorithm *rdpAlgorithm;
    int fsmState;
};

