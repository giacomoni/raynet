#include <inet/networklayer/common/L3AddressResolver.h>
#include  "../../transportlayer/contract/rdp/RdpSocket.h"
#include "RdpAppBase.h"

namespace inet {

void RdpAppBase::initialize(int stage)
{
    EV_TRACE << "RdpAppBase::initialize stage " << stage;
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_APPLICATION_LAYER) {
        // parameters
        const char *localAddress = par("localAddress");
        int localPort = par("localPort");

        socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
        socket.setCallback(this);
        socket.setOutputGate(gate("socketOut"));
    }
}

void RdpAppBase::handleMessageWhenUp(cMessage *msg)
{
    EV_TRACE << "RdpAppBase::handleMessageWhenUp" << endl;
    if (msg->isSelfMessage()) {
        handleTimer(msg);
    }
    else {
        socket.processMessage(msg);
    }
}

void RdpAppBase::connect()
{
    EV_TRACE << "RdpAppBase::connect" << endl;

    int numPacketsToSend = par("numPacketsToSend").intValue();

    // connect
    const char *connectAddress = par("connectAddress");
    int connectPort = par("connectPort");

    L3Address destination;
    L3AddressResolver().tryResolve(connectAddress, destination);

    // added by MOH
    const char *srcAddress = par("localAddress");
    L3Address localAddress;
    L3AddressResolver().tryResolve(srcAddress, localAddress);

    if (destination.isUnspecified()) {
        EV_ERROR << "Connecting to " << connectAddress << " port=" << connectPort << ": cannot resolve destination address\n";
    }
    else {
        EV_INFO << "Connecting to " << connectAddress << "(" << destination << ") port=" << connectPort << endl;

        socket.connect(localAddress, destination, connectPort, numPacketsToSend);
        EV_INFO << "Connecting to mmmmm" << connectAddress << "(" << destination << ") port=" << connectPort << endl;
    }
}

void RdpAppBase::close()
{
    EV_INFO << "issuing CLOSE command\n";
    socket.close();
}

void RdpAppBase::socketEstablished(RdpSocket*)
{
    // *redefine* to perform or schedule first sending
    EV_INFO << "connected" << endl;
}

void RdpAppBase::socketDataArrived(RdpSocket*, Packet *msg, bool)
{
    // *redefine* to perform or schedule next sending
    delete msg;
}

void RdpAppBase::socketPeerClosed(RdpSocket *socket_)
{
    throw cRuntimeError("RdpAppBase::socketPeerClosed(): never called");
}

void RdpAppBase::socketClosed(RdpSocket*)
{
    // *redefine* to start another session etc.
    EV_INFO << "connection closed" << endl;
}

void RdpAppBase::socketFailure(RdpSocket*, int code)
{
    // subclasses may override this function, and add code try to reconnect after a delay.
    EV_WARN << "connection broken\n";
}

void RdpAppBase::finish()
{
    std::string modulePath = getFullPath();
}

} // namespace inet
