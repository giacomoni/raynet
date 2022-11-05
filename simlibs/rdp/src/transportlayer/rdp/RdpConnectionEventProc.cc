#include <string.h>

#include "../contract/rdp/RdpCommand_m.h"
#include "../rdp/rdp_common/RdpHeader.h"
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"
#include "RdpReceiveQueue.h"
namespace inet {

namespace rdp {

//
// Event processing code
//
void RdpConnection::process_OPEN_ACTIVE(RdpEventCode &event, RdpCommand *RdpCommand, cMessage *msg)
{
    RdpOpenCommand *openCmd = check_and_cast<RdpOpenCommand*>(RdpCommand);
    L3Address localAddr, remoteAddr;
    int localPort, remotePort;
    switch (fsm.getState()) {
        case RDP_S_INIT:
            initConnection(openCmd);
            // store local/remote socket
            state->active = true;
            localAddr = openCmd->getLocalAddr();
            remoteAddr = openCmd->getRemoteAddr();
            localPort = openCmd->getLocalPort();
            remotePort = openCmd->getRemotePort();
            state->numPacketsToSend = openCmd->getNumPacketsToSend();
            if (remoteAddr.isUnspecified() || remotePort == -1){
                throw cRuntimeError(rdpMain, "Error processing command OPEN_ACTIVE: remote address and port must be specified");
            }
            if (localPort == -1) {
                localPort = rdpMain->getEphemeralPort();
            }
            EV_DETAIL << "process_OPEN_ACTIVE OPEN: " << localAddr << ":" << localPort << " --> " << remoteAddr << ":" << remotePort << endl;
            rdpMain->addSockPair(this, localAddr, remoteAddr, localPort, remotePort);
            sendEstabIndicationToApp();
            sendQueue->init(state->numPacketsToSend, B(1460)); //added B
            sendInitialWindow();
            break;
        default:
            throw cRuntimeError(rdpMain, "Error processing command OPEN_ACTIVE: connection already exists");
    }
    delete openCmd;
    delete msg;
}

void RdpConnection::process_OPEN_PASSIVE(RdpEventCode &event, RdpCommand *rdpCommand, cMessage *msg)
{
    RdpOpenCommand *openCmd = check_and_cast<RdpOpenCommand*>(rdpCommand);
    L3Address localAddr;
    int localPort;
    switch (fsm.getState()) {
        case RDP_S_INIT:
            initConnection(openCmd);
            state->active = false;
            localAddr = openCmd->getLocalAddr();
            localPort = openCmd->getLocalPort();
            if (localPort == -1)
                throw cRuntimeError(rdpMain, "Error processing command OPEN_PASSIVE: local port must be specified");
            rdpMain->addSockPair(this, localAddr, L3Address(), localPort, -1);
            break;
        default:
            throw cRuntimeError(rdpMain, "Error processing command OPEN_PASSIVE: connection already exists");
    }
    delete openCmd;
    delete msg;
}

} // namespace rdp

} // namespace inet

