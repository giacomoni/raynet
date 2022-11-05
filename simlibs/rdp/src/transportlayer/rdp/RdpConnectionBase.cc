#include <string.h>
#include <assert.h>

#include "../rdp/rdp_common/RdpHeader.h"
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"
#include "RdpReceiveQueue.h"

using namespace std;
namespace inet {

namespace rdp {
Define_Module(RdpConnection);

simsignal_t RdpConnection::trimmedHeadersSignal = registerSignal("trimmedHeaders");

RdpStateVariables::RdpStateVariables()
{
    internal_request_id = 0;
    request_id = 0;  // source block number (8-bit unsigned integer)

    numPacketsToGet = 0;
    numPacketsToSend = 0;
    congestionInWindow = false;

    lastPullTime = SIMTIME_ZERO;

    numRcvTrimmedHeader = 0;
    numberReceivedPackets = 0;
    numberSentPackets = 0;
    IW = 0; // send the initial window (12 Packets as in RDP) IWWWWWWWWWWWW
    receivedPacketsInWindow = 0;
    sentPullsInWindow = 0;
    additiveIncreasePackets = 1;
    slowStartState = true;
    outOfWindowPackets = 0;
    waitToStart = false;
    ssthresh = 0;
    connFinished = false;
    isfinalReceivedPrintedOut = false;
    numRcvdPkt = 0;
    delayedNackNo = 0;
    connNotAddedYet = true;
    cwnd = 0;
    sendPulls = true;
    active = false;
    pacingTime = 0;

    sRtt = SIMTIME_ZERO;
    minRtt  = SIMTIME_ZERO;
    latestRtt = SIMTIME_ZERO;
    rttvar = SIMTIME_ZERO;

    sRttStep = SIMTIME_ZERO;
    minRttStep = SIMTIME_ZERO;
    rttvarStep = SIMTIME_ZERO;

      //RTT - Header
    sRttHeader = SIMTIME_ZERO;
    minRttHeader = SIMTIME_ZERO;
    latestRttHeader = SIMTIME_ZERO;
    rttvarHeader = SIMTIME_ZERO;
    //RTT Step - Header
    sRttStepHeader = SIMTIME_ZERO;
    minRttStepHeader = SIMTIME_ZERO;
    rttvarStepHeader = SIMTIME_ZERO;

    bandwidthEstimator.setWindowSize(10);
    rttPropEstimator.setWindowSize(10);

    lastDataPacketArrived = 0;
}

std::string RdpStateVariables::str() const
{
    std::stringstream out;
    return out.str();
}

std::string RdpStateVariables::detailedInfo() const
{
    std::stringstream out;
    out << "active=" << active << "\n";
    return out.str();
}

void RdpConnection::initConnection(Rdp *_mod, int _socketId)
{
    Enter_Method_Silent
    ();

    rdpMain = _mod;
    socketId = _socketId;
    paceTimerMsg = new cMessage("paceTimerMsg");
    fsm.setName(getName());
    fsm.setState(RDP_S_INIT);

    // queues and algorithm will be created on active or passive open
}

RdpConnection::~RdpConnection()
{
    cancelAndDelete(paceTimerMsg);
    std::list<PacketsToSend>::iterator iter;  // received iterator

    while (!receivedPacketsList.empty()) {
        delete receivedPacketsList.front().msg;
        receivedPacketsList.pop_front();
    }
    delete sendQueue;
    delete receiveQueue;
    delete rdpAlgorithm;
    delete state;
}

void RdpConnection::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (!processTimer(msg))
            EV_INFO << "\nConnection Attempted Removal!\n";
    }
    else
        throw cRuntimeError("model error: RdpConnection allows only self messages");
}

bool RdpConnection::processTimer(cMessage *msg)
{
    printConnBrief();
    EV_DETAIL << msg->getName() << " timer expired\n";

    // first do actions
    RdpEventCode event;
    event = RDP_E_IGNORE;

    if(msg == paceTimerMsg){
        sendRequestFromPullsQueue();
        if (pullQueue.getByteLength() > 0) {
            schedulePullTimer(state->pacingTime);
        }
    }
    // then state transitions
    return performStateTransition(event);
}

void RdpConnection::schedulePullTimer(double time)
{
    Enter_Method("schedulePullTimer");
    if(!paceTimerMsg->isScheduled()){
        take(paceTimerMsg);
        scheduleAt(simTime() + time, paceTimerMsg);
    }
}

void RdpConnection::activatePullTimer()
{
    Enter_Method("activatePullTimer");
    //Do nothing if the timer is scheduled.
    //If it is not scheduled, schedule the next PR based on the
    // last PR sent timestamp and current pace
    if(!paceTimerMsg->isScheduled()){
        if(state->lastPullTime != SIMTIME_ZERO){
            simtime_t timeElapsedSinceLastSent = simTime()- state->lastPullTime;
            if (timeElapsedSinceLastSent.dbl() < state->pacingTime){
                take(paceTimerMsg);
                scheduleAt(simTime() + (state->pacingTime - timeElapsedSinceLastSent.dbl()), paceTimerMsg);
            }else{
                take(paceTimerMsg);
                scheduleAt(simTime(), paceTimerMsg);
            }
            }
        else{
            take(paceTimerMsg);
            scheduleAt(simTime(), paceTimerMsg);
        }
    }
}

bool RdpConnection::processrdpsegment(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address segSrcAddr, L3Address segDestAddr)
{
    Enter_Method_Silent();

    printConnBrief();
    RdpEventCode event = process_RCV_SEGMENT(packet, rdpseg, segSrcAddr, segDestAddr);
    // then state transitions
    return performStateTransition(event);
}

bool RdpConnection::processAppCommand(cMessage *msg)
{
    Enter_Method_Silent
    ();

    printConnBrief();

    RdpCommand *rdpCommand = check_and_cast_nullable<RdpCommand*>(msg->removeControlInfo());
    RdpEventCode event = preanalyseAppCommandEvent(msg->getKind());
    EV_INFO << "App command eventName: " << eventName(event) << "\n";
    switch (event) {
        case RDP_E_OPEN_ACTIVE:
            process_OPEN_ACTIVE(event, rdpCommand, msg);
            break;

        case RDP_E_OPEN_PASSIVE:
            process_OPEN_PASSIVE(event, rdpCommand, msg);
            break;

        default:
            throw cRuntimeError(rdpMain, "wrong event code");
    }

    // then state transitions
    return performStateTransition(event);
}

RdpEventCode RdpConnection::preanalyseAppCommandEvent(int commandCode)
{
    switch (commandCode) {
        case RDP_C_OPEN_ACTIVE:
            return RDP_E_OPEN_ACTIVE;

        case RDP_C_OPEN_PASSIVE:
            return RDP_E_OPEN_PASSIVE;

        default:
            throw cRuntimeError(rdpMain, "Unknown message kind in app command");
    }
}

bool RdpConnection::performStateTransition(const RdpEventCode &event)
{
    ASSERT(fsm.getState() != RDP_S_CLOSED); // closed connections should be deleted immediately

    if (event == RDP_E_IGNORE) {    // e.g. discarded segment
        EV_DETAIL << "Staying in state: " << stateName(fsm.getState()) << " (no FSM event)\n";
        return true;
    }

    // state machine
    // TBD add handling of connection timeout event (KEEP-ALIVE), with transition to CLOSED
    // Note: empty "default:" lines are for gcc's benefit which would otherwise spit warnings
    int oldState = fsm.getState();

    switch (fsm.getState()) {
        case RDP_S_INIT:
            switch (event) {
                case RDP_E_OPEN_PASSIVE:
                    FSM_Goto(fsm, RDP_S_LISTEN);
                    break;

                case RDP_E_OPEN_ACTIVE:
                    FSM_Goto(fsm, RDP_S_ESTABLISHED);
                    break;

                default:
                    break;
            }
            break;

        case RDP_S_LISTEN:
            switch (event) {
                case RDP_E_OPEN_ACTIVE:
                    FSM_Goto(fsm, RDP_S_SYN_SENT);
                    break;

                case RDP_E_RCV_SYN:
                    FSM_Goto(fsm, RDP_S_SYN_RCVD);
                    break;

                default:
                    break;
            }
            break;

        case RDP_S_SYN_RCVD:
            switch (event) {

                default:
                    break;
            }
            break;

        case RDP_S_SYN_SENT:
            switch (event) {

                case RDP_E_RCV_SYN:
                    FSM_Goto(fsm, RDP_S_SYN_RCVD);
                    break;

                default:
                    break;
            }
            break;

        case RDP_S_ESTABLISHED:
            switch (event) {

                default:
                    break;
            }
            break;

        case RDP_S_CLOSED:
            break;
    }

    if (oldState != fsm.getState()) {
        EV_INFO << "Transition: " << stateName(oldState) << " --> " << stateName(fsm.getState()) << "  (event was: " << eventName(event) << ")\n";
        EV_DEBUG_C("testing") << rdpMain->getName() << ": " << stateName(oldState) << " --> " << stateName(fsm.getState()) << "  (on " << eventName(event) << ")\n";

        // cancel timers, etc.
        stateEntered(fsm.getState(), oldState, event);
    }
    else {
        EV_DETAIL << "Staying in state: " << stateName(fsm.getState()) << " (event was: " << eventName(event) << ")\n";
    }

    return fsm.getState() != RDP_S_CLOSED;
}

void RdpConnection::stateEntered(int state, int oldState, RdpEventCode event)
{
    // cancel timers
    switch (state) {
        case RDP_S_INIT:
            // we'll never get back to INIT
            break;

        case RDP_S_LISTEN:
            // we may get back to LISTEN from SYN_RCVD
            break;

        case RDP_S_SYN_RCVD:
        case RDP_S_SYN_SENT:
            break;

        case RDP_S_ESTABLISHED:
            // we're in ESTABLISHED, these timers are no longer needed
            // RDP_I_ESTAB notification moved inside event processing
            break;

        case RDP_S_CLOSED:
            sendIndicationToApp(RDP_I_CLOSED);
            // all timers need to be cancelled
            rdpAlgorithm->connectionClosed();
            break;
    }
}
} // namespace RDP
} // namespace inet

