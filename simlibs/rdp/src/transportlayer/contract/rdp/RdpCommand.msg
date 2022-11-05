

import inet.common.INETDefs;
import inet.networklayer.common.IpProtocolId;
import inet.networklayer.common.L3Address;

namespace inet;
//
// NDPR command codes, sent by the application to NDPR. These constants
// should be set as message kind on a message sent to the NDPR entity.
//
// @see ~NDPRCommand, ~NDPROpenCommand, ~IRaptoNDP
//
enum RdpCommandCode
{
    RDP_C_OPEN_ACTIVE = 1;   // active open (must carry ~RdpOpenCommand)
    RDP_C_OPEN_PASSIVE = 2;  // passive open (must carry ~RdpOpenCommand)
    RDP_C_ACCEPT = 3;
}

//
// NDP indications, sent by NDP to the application. NDP will set these
// constants as message kind on messages it sends to the application.
//
// @see ~RdpCommand, ~RdpStatusInfo, ~INDP
//
enum RdpStatusInd
{
    RDP_I_DATA = 1;              // data packet (set on data packet)
    RDP_I_AVAILABLE = 3;         // conncetion available
    RDP_I_ESTABLISHED = 4;       // connection established
    RDP_I_PEER_CLOSED = 5;       // FIN received from remote NDP
    RDP_I_CLOSED = 6;            // connection closed normally (via FIN exchange)
}

class RdpCommand extends cObject
{
   int userId = -1;	// id than can be freely used by the app
   unsigned int  numRcvTrimmedHeader = 0;   
}

class RdpOpenCommand extends RdpCommand
{
    L3Address localAddr; 
    L3Address remoteAddr;
    int localPort = -1;      
    int remotePort = -1;      
    string rdpAlgorithmClass;
    unsigned int numPacketsToSend;
}

class RdpAvailableInfo extends RdpCommand
{
    L3Address localAddr;
    L3Address remoteAddr;
    int localPort;
    int remotePort;
    int newSocketId;
}

class RdpConnectInfo extends RdpCommand
{
    L3Address localAddr;
    L3Address remoteAddr;
    int localPort;
    int remotePort;
}

class RdpStatusInfo extends RdpCommand
{
    int state;
    string stateName;
    L3Address localAddr;
    L3Address remoteAddr;
    int localPort;
    int remotePort;
}
