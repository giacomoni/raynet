
#include "../rdp/RdpSocket.h"

#include <inet/common/packet/Message.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/applications/common/SocketTag_m.h>
#include <inet/common/Protocol.h>

namespace inet {

RdpSocket::RdpSocket()
{
    // don't allow user-specified connIds because they may conflict with
    // automatically assigned ones.
    connId = getEnvir()->getUniqueNumber();
}

RdpSocket::~RdpSocket()
{
    if (cb) {
        cb->socketDeleted(this);
        cb = nullptr;
    }
}

const char* RdpSocket::stateName(RdpSocket::State state)
{
#define CASE(x)    case x: \
        s = #x; break
    const char *s = "unknown";
    switch (state) {
        CASE(NOT_BOUND)
;            CASE(BOUND);
            CASE(LISTENING);
            CASE(CONNECTING);
            CASE(CONNECTED);
        }
    return s;
#undef CASE
}

void RdpSocket::sendToRDP(cMessage *msg, int connId)
{
    if (!gateToRdp)
        throw cRuntimeError("RdpSocket: setOutputGate() must be invoked before socket can be used");

    auto &tags = getTags(msg);
    tags.addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::rdp);
    tags.addTagIfAbsent<SocketReq>()->setSocketId(connId == -1 ? this->connId : connId);
    check_and_cast<cSimpleModule*>(gateToRdp->getOwnerModule())->send(msg, gateToRdp);
}

void RdpSocket::bind(L3Address lAddr, int lPort)
{
    if (sockstate != NOT_BOUND)
        throw cRuntimeError("RdpSocket::bind(): socket already bound");

    // allow -1 here, to make it possible to specify address only
    if ((lPort < 0 || lPort > 65535) && lPort != -1)
        throw cRuntimeError("RdpSocket::bind(): invalid port number %d", lPort);
    localAddr = lAddr;
    localPrt = lPort;
    sockstate = BOUND;
}

void RdpSocket::listen(bool fork)
{
    if (sockstate != BOUND)
        throw cRuntimeError(sockstate == NOT_BOUND ? "RdpSocket: must call bind() before listen()" : "RdpSocket::listen(): connect() or listen() already called");

    auto request = new Request("PassiveOPEN", RDP_C_OPEN_PASSIVE);
    RdpOpenCommand *openCmd = new RdpOpenCommand();
    openCmd->setLocalAddr(localAddr);
    openCmd->setLocalPort(localPrt);
    openCmd->setRdpAlgorithmClass(rdpAlgorithmClass.c_str());
    request->setControlInfo(openCmd);
    sendToRDP(request);
    sockstate = LISTENING;
}

void RdpSocket::accept(int socketId)
{
    throw cRuntimeError("RdpSocket::accept(): never called");
}

void RdpSocket::connect(L3Address localAddress, L3Address remoteAddress, int remotePort, unsigned int numPacketsToSend)
{
    if (sockstate != NOT_BOUND && sockstate != BOUND)
        throw cRuntimeError("RdpSocket::connect(): connect() or listen() already called (need renewSocket()?)");

    if (remotePort < 0 || remotePort > 65535)
        throw cRuntimeError("RdpSocket::connect(): invalid remote port number %d", remotePort);

    auto request = new Request("ActiveOPEN", RDP_C_OPEN_ACTIVE);
    localAddr = localAddress;
    remoteAddr = remoteAddress;
    remotePrt = remotePort;

    RdpOpenCommand *openCmd = new RdpOpenCommand();
    openCmd->setLocalAddr(localAddr);
    openCmd->setLocalPort(localPrt);
    openCmd->setRemoteAddr(remoteAddr);
    openCmd->setRemotePort(remotePrt);
    openCmd->setRdpAlgorithmClass(rdpAlgorithmClass.c_str());
    openCmd->setNumPacketsToSend(numPacketsToSend);
    request->setControlInfo(openCmd);
    sendToRDP(request);
    sockstate = CONNECTING;
    EV_INFO << "Socket Connection Finished" << endl;
}

void RdpSocket::send(Packet *msg)
{
    throw cRuntimeError("RdpSocket::send(): never called by application - hack where RDP handles all data");
}

void RdpSocket::close()
{
    throw cRuntimeError("RdpSocket::close(): never called by application");
}

void RdpSocket::abort()
{
    throw cRuntimeError("RdpSocket::abort(): never called by application - hack where RDP handles all data");
}

void RdpSocket::destroy()
{
    throw cRuntimeError("RdpSocket::destroy(): never called by application - hack where RDP handles all data");
}

void RdpSocket::renewSocket()
{
    throw cRuntimeError("RdpSocket::renewSocket(): not needed as the socket should never be closed to begin with");
}

bool RdpSocket::isOpen() const
{
    throw cRuntimeError("RdpSocket::isOpen(): never called");
}

bool RdpSocket::belongsToSocket(cMessage *msg) const
{
    auto &tags = getTags(msg);
    auto socketInd = tags.findTag<SocketInd>();
    return socketInd != nullptr && socketInd->getSocketId() == connId;
}

void RdpSocket::setCallback(ICallback *callback)
{
    cb = callback;
}

void RdpSocket::processMessage(cMessage *msg)
{
    ASSERT(belongsToSocket(msg));
    RdpConnectInfo *connectInfo;

    switch (msg->getKind()) {
        case RDP_I_DATA:
            if (cb)
                cb->socketDataArrived(this, check_and_cast<Packet*>(msg), false); // see RdpBasicClientApp::socketDataArrived
            else
                delete msg;

            break;

        case RDP_I_ESTABLISHED:
            // Note: this code is only for sockets doing active open, and nonforking
            // listening sockets. For a forking listening sockets, RDP_I_ESTABLISHED
            // carries a new connId which won't match the connId of this RdpSocket,
            // so you won't get here. Rather, when you see RDP_I_ESTABLISHED, you'll
            // want to create a new RdpSocket object via new RdpSocket(msg).
            sockstate = CONNECTED;
            connectInfo = check_and_cast<RdpConnectInfo*>(msg->getControlInfo());
            localAddr = connectInfo->getLocalAddr();
            remoteAddr = connectInfo->getRemoteAddr();
            localPrt = connectInfo->getLocalPort();
            remotePrt = connectInfo->getRemotePort();
            if (cb)
                cb->socketEstablished(this);
            delete msg;
            break;
        default:
            throw cRuntimeError("RdpSocket: invalid msg kind %d, one of the RDP_I_xxx constants expected", msg->getKind());
    }
}

} // namespace inet

