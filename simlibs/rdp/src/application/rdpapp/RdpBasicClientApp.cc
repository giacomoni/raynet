#include <iostream>
#include <random>
#include <inet/common/lifecycle/ModuleOperations.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/TimeTag_m.h>

#include "GenericAppMsgRdp_m.h"
#include "RdpBasicClientApp.h"
namespace inet {

#define MSGKIND_CONNECT    0

Define_Module(RdpBasicClientApp);

RdpBasicClientApp::~RdpBasicClientApp()
{
    cancelAndDelete(timeoutMsg);
}

void RdpBasicClientApp::initialize(int stage)
{
    EV_TRACE << "RdpBasicClientApp::initialize stage " << stage << endl;
    RdpAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        //timeoutMsg = new cMessage("timer");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
            timeoutMsg = new cMessage("timer");
            nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
            if (isNodeUp()) {
                timeoutMsg->setKind(MSGKIND_CONNECT);
                scheduleAt(startTime, timeoutMsg);
            }
    }
    // TODO update timer to make it more up to date
}

bool RdpBasicClientApp::isNodeUp() {
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

void RdpBasicClientApp::handleStartOperation(LifecycleOperation *operation)
{
    simtime_t now = simTime();
    simtime_t start = std::max(startTime, now);
    if (timeoutMsg && ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))) {
        timeoutMsg->setKind(MSGKIND_CONNECT);
        scheduleAt(start, timeoutMsg);
    }
}

void RdpBasicClientApp::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
    if (socket.getState() == RdpSocket::CONNECTED || socket.getState() == RdpSocket::CONNECTING){
        close();
    }
}

void RdpBasicClientApp::handleCrashOperation(LifecycleOperation *operation)
{
    throw cRuntimeError("RdpBasicClientApp::handleCrashOperation - not implemented");
}

void RdpBasicClientApp::handleTimer(cMessage *msg)
{
    // Added MOH send requests based on a timer
    switch (msg->getKind()) {
        case MSGKIND_CONNECT:
            connect();    // active OPEN
            break;
        default:
            throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
}

void RdpBasicClientApp::socketEstablished(RdpSocket *socket)
{
    RdpAppBase::socketEstablished(socket);
}

void RdpBasicClientApp::rescheduleOrDeleteTimer(simtime_t d, short int msgKind)
{
    cancelEvent(timeoutMsg);

    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        timeoutMsg->setKind(msgKind);
        scheduleAt(d, timeoutMsg);
    }
    else {
        delete timeoutMsg;
        timeoutMsg = nullptr;
    }
}

void RdpBasicClientApp::close()
{
    RdpAppBase::close();
    cancelEvent(timeoutMsg);
}
void RdpBasicClientApp::socketClosed(RdpSocket *socket)
{
    RdpAppBase::socketClosed(socket);
}

void RdpBasicClientApp::socketFailure(RdpSocket *socket, int code)
{
    RdpAppBase::socketFailure(socket, code);
    // reconnect after a delay
    if (timeoutMsg) {
        simtime_t d = simTime() + (simtime_t) par("reconnectInterval");
        rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
    }
}

}    // namespace inet
