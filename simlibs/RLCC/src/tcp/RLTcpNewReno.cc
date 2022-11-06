//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifdef RLTCP

#include "RLTcpNewReno.h"

#define RTT_MULTIPLIER 1.7 // multiplier for the srtt when setting MI length
#define MI_MINIMUM_LENGTH 0.005 // default length of MI

//These value have  been copied from TcpBaseAlg because they are not visibile from this file.
#define MIN_REXMIT_TIMEOUT 0.01
#define MAX_REXMIT_TIMEOUT 240

using namespace inet::tcp;
using namespace inet;

namespace learning {

Register_Class(RLTcpNewReno);



RLTcpNewReno::RLTcpNewReno() :
        TcpNewReno(), RLInterface() {
}

RLTcpNewReno::~RLTcpNewReno() {
    if (monitorInterval)
        delete cancelEvent(monitorInterval);

    getSimulation()->getSystemModule()->unsubscribe(stringId.c_str(), (cListener*) this);
    getSimulation()->getSystemModule()->unsubscribe("actionResponse", (cListener*) this);
}

void RLTcpNewReno::initialize() {
    int _stateSize;
    int _maxObsCount;
    double _ewmaWeight;

    // check underlying TcpConnection to decide if this a paced connection or not
    (strcmp(this->conn->getNedTypeName(), "rltcp.transportlayer.PacedTcpConnection") == 0) ? isConnectionPaced = true : isConnectionPaced = false;

    maxLearnWindow = this->conn->getTcpMain()->par("maxWindow");
    _stateSize = this->conn->getTcpMain()->par("stateSize");
    _maxObsCount = this->conn->getTcpMain()->par("maxObsCount");
    _ewmaWeight = this->conn->getTcpMain()->par("ewmaWeight");

    miHandler = MonitorIntervalsHandler();
    miHandler.currentMi = nullptr;

    monitorInterval = new cMessage("MONITORINTERVAL");

    // provide the RLInterface with a cComponent API (to use signaling functionality)
    setOwner((cComponent*) conn->getTcpMain());

    RLInterface::initialize(_stateSize, _maxObsCount);
    TcpNewReno::initialize();

    // instanitate the EWMA filters
    interArrivalEwmaPtr = std::make_shared<Ewma>(_ewmaWeight);
    interSendEwmaPtr = std::make_shared<Ewma>(_ewmaWeight);
    rttEwmaPtr = std::make_shared<Ewma>(_ewmaWeight);

    dupAcks = 0;

    lastMIReward = -1;
    lastMIavgRTT = -1;
    lastMiAction = 0;

    throughputSignal = conn->registerSignal("throughput");
    actionSignal = conn->registerSignal("action");
    dupAcksSignal = conn->registerSignal("dupAcks");
    rttGradientSignal = conn->registerSignal("rttGradient");
    tickSignal = conn->registerSignal("tick");
    miQueueSizeSignal = conn->registerSignal("MIQueueSize");
}

void RLTcpNewReno::established(bool active) {
    TcpNewReno::established(active);

    if (active) {
        setStringId(conn->getLocalAddress().str());
        this->isActive = active;
        conn->emit(cwndSignal, state->snd_cwnd);
        conn->emit(dupAcksSignal, dupAcks);
    }
}

bool RLTcpNewReno::sendData(bool sendCommandInvoked) {
    bool b;
    uint32 oldSndMax, newSndMax;

    oldSndMax = state->snd_max;
    b = TcpBaseAlg::sendData(sendCommandInvoked);
    newSndMax = state->snd_max;

    EV_INFO << "Sent " << newSndMax - oldSndMax << " bytes" << std::endl;

    // action needed only if we sent new data
    if (newSndMax - oldSndMax > 0) {
        // start ticking if we are not ticking
        if (miHandler.currentMi == nullptr) {

            double _action =  (double) state->snd_cwnd / (double) (maxLearnWindow * state->snd_mss);
            miHandler.currentMi = std::make_shared<MonitorInterval>(miHandler.counter, oldSndMax, simTime(), false, false, _action);
            miHandler.counter++;
            lastMiAction = _action;

            EV_INFO << "MI " << miHandler.currentMi->miNumber << " starting with snd_max: " << oldSndMax << std::endl;
            EV_INFO << "Scheduling next tick in " + std::to_string(MI_MINIMUM_LENGTH) + "s, at " << simTime() + MI_MINIMUM_LENGTH << std::endl;

            // schedule next tick
            conn->scheduleTimeout(monitorInterval, MI_MINIMUM_LENGTH);

            //We normally set the pace only when cwnd is changed. However, this code is executed only once, when the firts
            //MI is instantiated and we need to sate the pace accordingly if connection is paced.
            if (isConnectionPaced) {
                double paceD = MI_MINIMUM_LENGTH / ((double) state->snd_cwnd / (double) state->snd_mss);
                auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
                pacedConn->intersendingTime = paceD;
                pacedConn->paceValueVec.record(paceD);

            }
        }

        //Update mi.lastSeqSent everytime we send new data, not only when we tick.
        miHandler.currentMi->lastSndMax = state->snd_max;

        // mark first packet in the current MI if new data is sent for the first time in the MI
        if (!miHandler.currentMi->isFirstSegmentSent) {
            EV_INFO << "First segment sent for MI " << miHandler.currentMi->miNumber << std::endl;
            miHandler.currentMi->isFirstSegmentSent = true;
            miHandler.currentMi->firstSegmentTime = simTime();
        }

        // We check whether new data has been sent by looking at miHandler.currentMi->isFirstSegmentSent, which is true when the first segment of current mi has been sent.
        if (miHandler.currentMi->deferredTicking) {
            EV_INFO << "executing deferred tick" << std::endl;
            ASSERT(miHandler.currentMi->isFirstSegmentSent);
            miHandler.currentMi->deferredTicking = false;
            tick();
        }
    }
    else {
        EV_INFO << "sendData: did not send new data.." << std::endl;
    }

    return b;
}

void RLTcpNewReno::receivedDataAck(uint32 firstSeqAcked) {

    //Since we measure RTT useing TS option, we want to make sure this is on.
    ASSERT(state->ts_enabled);
    if (state->lossRecovery) {
        if (seqGE(state->snd_una - 1, state->recover)) {
            // Exit Fast Recovery: deflating cwnd
            //
            // option (1): set cwnd to min (ssthresh, FlightSize + SMSS)
            uint32 flight_size = state->snd_max - state->snd_una;
            state->snd_cwnd = std::min(state->ssthresh, flight_size + state->snd_mss);
            EV_INFO << "Fast Recovery - Full ACK received: Exit Fast Recovery, setting cwnd to " << state->snd_cwnd << "\n";
            // option (2): set cwnd to ssthresh
            // state->snd_cwnd = state->ssthresh;
            // tcpEV << "Fast Recovery - Full ACK received: Exit Fast Recovery, setting cwnd to ssthresh=" << state->ssthresh << "\n";
            // TODO - If the second option (2) is selected, take measures to avoid a possible burst of data (maxburst)!
            conn->emit(cwndSignal, state->snd_cwnd);

            state->lossRecovery = false;
            state->firstPartialACK = false;
            EV_INFO << "Loss Recovery terminated.\n";
        }
        else {
            // RFC 3782, page 5:
            // "Partial acknowledgements:
            // If this ACK does *not* acknowledge all of the data up to and
            // including "recover", then this is a partial ACK.  In this case,
            // retransmit the first unacknowledged segment.  Deflate the
            // congestion window by the amount of new data acknowledged by the
            // cumulative acknowledgement field.  If the partial ACK
            // acknowledges at least one SMSS of new data, then add back SMSS
            // bytes to the congestion window.  As in Step 3, this artificially
            // inflates the congestion window in order to reflect the additional
            // segment that has left the network.  Send a new segment if
            // permitted by the new value of cwnd.  This "partial window
            // deflation" attempts to ensure that, when Fast Recovery eventually
            // ends, approximately ssthresh amount of data will be outstanding
            // in the network.  Do not exit the Fast Recovery procedure (i.e.,
            // if any duplicate ACKs subsequently arrive, execute Steps 3 and 4
            // above).
            //
            // For the first partial ACK that arrives during Fast Recovery, also
            // reset the retransmit timer.  Timer management is discussed in
            // more detail in Section 4."

            EV_INFO << "Fast Recovery - Partial ACK received: retransmitting the first unacknowledged segment\n";
            // retransmit first unacknowledged segment
            conn->retransmitOneSegment(false);

            // deflate cwnd by amount of new data acknowledged by cumulative acknowledgement field
            state->snd_cwnd -= state->snd_una - firstSeqAcked;

            conn->emit(cwndSignal, state->snd_cwnd);

            EV_INFO << "Fast Recovery: deflating cwnd by amount of new data acknowledged, new cwnd=" << state->snd_cwnd << "\n";

            // if the partial ACK acknowledges at least one SMSS of new data, then add back SMSS bytes to the cwnd
            if (state->snd_una - firstSeqAcked >= state->snd_mss) {
                state->snd_cwnd += state->snd_mss;

                conn->emit(cwndSignal, state->snd_cwnd);

                EV_DETAIL << "Fast Recovery: inflating cwnd by SMSS, new cwnd=" << state->snd_cwnd << "\n";
            }

            // try to send a new segment if permitted by the new value of cwnd
            sendData(false);

            // reset REXMIT timer for the first partial ACK that arrives during Fast Recovery
            if (state->lossRecovery) {
                if (!state->firstPartialACK) {
                    state->firstPartialACK = true;
                    EV_DETAIL << "First partial ACK arrived during recovery, restarting REXMIT timer.\n";
                    restartRexmitTimer();
                }
            }
        }
    }
    else {
        // RFC 3782, page 13:
        // "When not in Fast Recovery, the value of the state variable "recover"
        // should be pulled along with the value of the state variable for
        // acknowledgments (typically, "snd_una") so that, when large amounts of
        // data have been sent and acked, the sequence space does not wrap and
        // falsely indicate that Fast Recovery should not be entered (Section 3,
        // step 1, last paragraph)."
        state->recover = (state->snd_una - 2);
    }

    //First, we check if the current ACK acknowledges all data sent in this monitor interval. We have to check if miHandler it's not a nullptr
    //because we can receive ACKs all data has been sent and the currentMI is a nullptr.
    if (miHandler.currentMi != nullptr && state->snd_una >= miHandler.currentMi->lastSndMax && miHandler.currentMi->isFirstSegmentSent) {
        //if so, we record the time
        miHandler.currentMi->ackForLastSegmentTime = simTime();
    }

    // Second, we check whether ALL data sent during any MI has been ACKed and we calculate thee throughput.
    for (auto mi = miHandler.incompleteMis.begin(); mi != miHandler.incompleteMis.end();) {
        // if all data has been acknowledged
        if (state->snd_una >= (*mi)->lastSndMax) {
            ASSERT(simTime() != (*mi)->firstSegmentTime);
            // compute throughput using info stored into the MI and the current time
            double throughput = ((*mi)->lastSndMax - (*mi)->firstSndMax) / (simTime() - (*mi)->firstSegmentTime);
            throughput = throughput / 1000000 * 8;

            // at this point, the reward assigned to the stp should be -1 since no throughput has been assigned yet.
            ASSERT((*mi)->step->r == -1);
            // set the reward value of the step
            (*mi)->step->r = throughput;
            cout << "Computed throughput of: " << throughput << ". Sending to replayBuffer. " << std::endl;
            owner->emit(dataSignal, &(*((*mi)->step)));

            // remove MI from queue
            mi = miHandler.incompleteMis.erase(mi);
            EV_INFO << "There are " << miHandler.incompleteMis.size() << " MIs in the queue" << std::endl;

            conn->emit(miQueueSizeSignal, miHandler.incompleteMis.size());
            conn->emit(throughputSignal, throughput);
        }
        else {
            ++mi;
        }
    }

    sendData(false);

//    **************************************************************************************************************************
    if (isActive) {
        EV_INFO << "Received ack up to: " << (state->snd_una - 1) << std::endl;

        // currently we do not support timestamps
        TcpTahoeRenoFamily::receivedDataAck(firstSeqAcked);
        if (state->dupacks >= DUPTHRESH) {    // DUPTHRESH = 3
            //
            // Perform Fast Recovery: set cwnd to ssthresh (deflating the window).
            //
            EV_INFO << "Fast Recovery: setting cwnd to ssthresh=" << state->ssthresh << "\n";
            state->snd_cwnd = state->ssthresh;

            conn->emit(cwndSignal, state->snd_cwnd);
        }
        if (state->sack_enabled && state->lossRecovery) {
            // RFC 3517, page 7: "Once a TCP is in the loss recovery phase the following procedure MUST
            // be used for each arriving ACK:
            //
            // (A) An incoming cumulative ACK for a sequence number greater than
            // RecoveryPoint signals the end of loss recovery and the loss
            // recovery phase MUST be terminated.  Any information contained in
            // the scoreboard for sequence numbers greater than the new value of
            // HighACK SHOULD NOT be cleared when leaving the loss recovery
            // phase."
            if (seqGE(state->snd_una, state->recoveryPoint)) {
                EV_INFO << "Loss Recovery terminated.\n";
                state->lossRecovery = false;
            }
            // RFC 3517, page 7: "(B) Upon receipt of an ACK that does not cover RecoveryPoint the
            //following actions MUST be taken:
            //
            // (B.1) Use Update () to record the new SACK information conveyed
            // by the incoming ACK.
            //
            // (B.2) Use SetPipe () to re-calculate the number of octets still
            // in the network."
            else {
                // update of scoreboard (B.1) has already be done in readHeaderOptions()
                conn->setPipe();

                // RFC 3517, page 7: "(C) If cwnd - pipe >= 1 SMSS the sender SHOULD transmit one or more
                // segments as follows:"
                if (((int) state->snd_cwnd - (int) state->pipe) >= (int) state->snd_mss) // Note: Typecast needed to avoid prohibited transmissions
                    conn->sendDataDuringLossRecoveryPhase(state->snd_cwnd);
            }
        }

        sendData(false);
    }
    else {
        TcpNewReno::receivedDataAck(firstSeqAcked);
    }
}

void RLTcpNewReno::receivedDuplicateAck() {
    if (isActive) {

        std::cout << "Amount of packets sackd:" << conn->rexmitQueue->getTotalAmountOfSackedBytes() / state->snd_mss << std::endl;
        std::cout << "Packets lost: " << conn->rexmitQueue->getNumOfDiscontiguousSacks(state->snd_una) << std::endl;

        dupAcks++;

        TcpTahoeRenoFamily::receivedDuplicateAck();
        if (state->dupacks == DUPTHRESH) {    // DUPTHRESH = 3
            if (!state->lossRecovery) {
                // RFC 3782, page 4:
                // "1) Three duplicate ACKs:
                // When the third duplicate ACK is received and the sender is not
                // already in the Fast Recovery procedure, check to see if the
                // Cumulative Acknowledgement field covers more than "recover".  If
                // so, go to Step 1A.  Otherwise, go to Step 1B."
                //
                // RFC 3782, page 6:
                // "Step 1 specifies a check that the Cumulative Acknowledgement field
                // covers more than "recover".  Because the acknowledgement field
                // contains the sequence number that the sender next expects to receive,
                // the acknowledgement "ack_number" covers more than "recover" when:
                //      ack_number - 1 > recover;"
                if (state->snd_una - 1 > state->recover) {
                    EV_INFO << "NewReno on dupAcks == DUPTHRESH(=3): perform Fast Retransmit, and enter Fast Recovery:";

                    // RFC 3782, page 4:
                    // "1A) Invoking Fast Retransmit:
                    // If so, then set ssthresh to no more than the value given in
                    // equation 1 below.  (This is equation 3 from [RFC2581]).
                    //      ssthresh = max (FlightSize / 2, 2*SMSS)           (1)
                    // In addition, record the highest sequence number transmitted in
                    // the variable "recover", and go to Step 2."
                    recalculateSlowStartThreshold();
                    state->recover = (state->snd_max - 1);
                    state->firstPartialACK = false;
                    state->lossRecovery = true;
                    EV_INFO << " set recover=" << state->recover;

                    // RFC 3782, page 4:
                    // "2) Entering Fast Retransmit:
                    // Retransmit the lost segment and set cwnd to ssthresh plus 3 * SMSS.
                    // This artificially "inflates" the congestion window by the number
                    // of segments (three) that have left the network and the receiver
                    // has buffered."
                    state->snd_cwnd = state->ssthresh + 3 * state->snd_mss;

                    conn->emit(cwndSignal, state->snd_cwnd);

                    EV_DETAIL << " , cwnd=" << state->snd_cwnd << ", ssthresh=" << state->ssthresh << "\n";
                    conn->retransmitOneSegment(false);

                    // RFC 3782, page 5:
                    // "4) Fast Recovery, continued:
                    // Transmit a segment, if allowed by the new value of cwnd and the
                    // receiver's advertised window."
                    sendData(false);
                }
                else {
                    EV_INFO << "NewReno on dupAcks == DUPTHRESH(=3): not invoking Fast Retransmit and Fast Recovery\n";

                    // RFC 3782, page 4:
                    // "1B) Not invoking Fast Retransmit:
                    // Do not enter the Fast Retransmit and Fast Recovery procedure.  In
                    // particular, do not change ssthresh, do not go to Step 2 to
                    // retransmit the "lost" segment, and do not execute Step 3 upon
                    // subsequent duplicate ACKs."
                }
            }
            EV_INFO << "NewReno on dupAcks == DUPTHRESH(=3): TCP is already in Fast Recovery procedure\n";
        }
        else if (state->dupacks > DUPTHRESH) {    // DUPTHRESH = 3
            if (state->lossRecovery) {
                // RFC 3782, page 4:
                // "3) Fast Recovery:
                // For each additional duplicate ACK received while in Fast
                // Recovery, increment cwnd by SMSS.  This artificially inflates the
                // congestion window in order to reflect the additional segment that
                // has left the network."
                state->snd_cwnd += state->snd_mss;

                conn->emit(cwndSignal, state->snd_cwnd);

                EV_DETAIL << "NewReno on dupAcks > DUPTHRESH(=3): Fast Recovery: inflating cwnd by SMSS, new cwnd=" << state->snd_cwnd << "\n";

                // RFC 3782, page 5:
                // "4) Fast Recovery, continued:
                // Transmit a segment, if allowed by the new value of cwnd and the
                // receiver's advertised window."
                sendData(false);
            }
        }
    }
}

void RLTcpNewReno::processTimer(cMessage *timer, TcpEventCode &event) {
    if (timer == rexmitTimer)
        processRexmitTimer(event);
    else if (timer == persistTimer)
        processPersistTimer(event);
    else if (timer == delayedAckTimer)
        processDelayedAckTimer(event);
    else if (timer == keepAliveTimer)
        processKeepAliveTimer(event);
    else if (timer == monitorInterval)
        processMonitorIntervalTimer(event);
    else
        throw cRuntimeError(timer, "unrecognised timer");
}

void RLTcpNewReno::processRexmitTimer(TcpEventCode &event) {
    if (isActive) {

        if (event == TCP_E_ABORT)
            return;

        // RFC 3782, page 6:
        // "6)  Retransmit timeouts:
        // After a retransmit timeout, record the highest sequence number
        // transmitted in the variable "recover" and exit the Fast Recovery
        // procedure if applicable."
        state->recover = (state->snd_max - 1);
        EV_INFO << "recover=" << state->recover << "\n";
        state->lossRecovery = false;
        state->firstPartialACK = false;
        EV_INFO << "Loss Recovery terminated.\n";

        // After REXMIT timeout TCP NewReno should start slow start with snd_cwnd = snd_mss.
        //
        // If calling "retransmitData();" there is no rexmit limitation (bytesToSend > snd_cwnd)
        // therefore "sendData();" has been modified and is called to rexmit outstanding data.
        //
        // RFC 2581, page 5:
        // "Furthermore, upon a timeout cwnd MUST be set to no more than the loss
        // window, LW, which equals 1 full-sized segment (regardless of the
        // value of IW).  Therefore, after retransmitting the dropped segment
        // the TCP sender uses the slow start algorithm to increase the window
        // from 1 full-sized segment to the new value of ssthresh, at which
        // point congestion avoidance again takes over."

        // begin Slow Start (RFC 2581)
        recalculateSlowStartThreshold();
        state->snd_cwnd = state->snd_mss;

        conn->emit(cwndSignal, state->snd_cwnd);

        EV_INFO << "Begin Slow Start: resetting cwnd to " << state->snd_cwnd << ", ssthresh=" << state->ssthresh << "\n";
        state->afterRto = true;
        conn->retransmitOneSegment(true);
    }
    else {
        TcpNewReno::processRexmitTimer(event);
    }
}

void RLTcpNewReno::processMonitorIntervalTimer(TcpEventCode &event) {
    if (miHandler.currentMi->isFirstSegmentSent)
        tick();
    else {
        EV_INFO << "Not a single packet sent yet for MI " << miHandler.currentMi->miNumber << ". Deferring ticking to when a packet is sent." << std::endl;
        miHandler.currentMi->deferredTicking = true;
    }
}

void RLTcpNewReno::tick() {
    ASSERT(miHandler.currentMi != nullptr);

    // log the tick
    conn->emit(tickSignal, 1);

    EV_INFO << "End of monitor interval number " << miHandler.currentMi->miNumber << std::endl;
    EV_INFO << "MI " << miHandler.currentMi->miNumber << " ending with: " << state->snd_max - 1 << std::endl;

    // get the new cwnd size from the agent
    act();

    //Create step AFTER we act, because we need the ne rlState, which is computed in act().

    // reward is -1 here because this will be computed when the ACK of the last segment sent within this MI will be received.
    shared_ptr<Step> step = std::make_shared<Step>(rlOldState, miHandler.currentMi->action, -1, rlState, false);
    miHandler.currentMi->step = step;

    // set the value of lastSeqSent of the current MI (snd_max).
    miHandler.currentMi->lastSndMax = state->snd_max;

    // set the ending time of the current MI
    miHandler.currentMi->miEnd = simTime();

    ASSERT(miHandler.currentMi->miEnd - miHandler.currentMi->miStart >= MI_MINIMUM_LENGTH);
    ASSERT(miHandler.currentMi->lastSndMax > miHandler.currentMi->firstSndMax);

    //First we check if all data of current MI has been ACKed. If so, we compute throughput, create step instance and send it to the agent straight away.
    //If not, we create a mock step instance and compute throughput later in time.
    if (miHandler.currentMi->ackForLastSegmentTime != SimTime::ZERO) {

        ASSERT(miHandler.currentMi->ackForLastSegmentTime != miHandler.currentMi->firstSegmentTime);

        double throughput = (miHandler.currentMi->lastSndMax - miHandler.currentMi->firstSndMax) / (miHandler.currentMi->ackForLastSegmentTime - miHandler.currentMi->firstSegmentTime);
        throughput = throughput / 1000000 * 8;
        // set the reward value of the step
        miHandler.currentMi->step->r = throughput;
        cout << "Computed throughput of: " << throughput << ". Sending to replayBuffer. " << std::endl;
        owner->emit(dataSignal, &(*(miHandler.currentMi->step)));
    }
    else {
        miHandler.addMI(miHandler.currentMi);
    }

    conn->emit(miQueueSizeSignal, miHandler.incompleteMis.size());

    EV_INFO << "There are " << miHandler.incompleteMis.size() << " MIs in the queue" << std::endl;

    if (conn->getSendQueue()->getBytesAvailable(state->snd_max) == 0) {
        // no more data in the queue - stop ticking
        EV_INFO << "No More data." << std::endl;
        miHandler.currentMi = nullptr;

        // TODO: why do we need to cancel this? it is either called in the handler or it is deferred and timer is not running
        cancelEvent(monitorInterval);
    }
    else {
        EV_INFO << "Creating new MI instance number " << miHandler.counter << std::endl;

        double _action = (double) state->snd_cwnd / (double) (maxLearnWindow * state->snd_mss);
        // here, state->snd_max is the correct value for instantiating miHandler.currentMi
        miHandler.currentMi = std::make_shared<MonitorInterval>(miHandler.counter, state->snd_max, simTime(), false, false, _action);
        miHandler.counter++;
        lastMiAction = _action;

        EV_INFO << "MI " << miHandler.currentMi->miNumber << " starting with: " << state->snd_max << std::endl;

        ASSERT(MI_MINIMUM_LENGTH > 0);

        if (lastMIavgRTT > MI_MINIMUM_LENGTH) {
            EV_INFO << "Scheduling next tick in " + std::to_string(RTT_MULTIPLIER * lastMIavgRTT) + "s, at " << simTime() + RTT_MULTIPLIER * lastMIavgRTT << std::endl;
            conn->scheduleTimeout(monitorInterval, RTT_MULTIPLIER * lastMIavgRTT);
        }
        else {
            EV_INFO << "Scheduling next tick in " + std::to_string(MI_MINIMUM_LENGTH) + "s, at " << simTime() + MI_MINIMUM_LENGTH << std::endl;
            conn->scheduleTimeout(monitorInterval, MI_MINIMUM_LENGTH);
        }
    }
}

void RLTcpNewReno::computeObservation(std::vector<double> &obs) {
    // calculate gradient of rtt received, if at least two RTT measurements have been done.
    double rttGradient;

    if (rttsMi.size() > 1) {
        rttGradient = slope(rttsMiTimes, rttsMi);
        if (abs(rttGradient) < 0.0001) {
            rttGradient = 0;
        }
    }
    else {
        rttGradient = 0;
    }

    ASSERT(!isnan(rttGradient));

    if (rttsMi.size() > 0) {
        //compute avg of RTT in the last MI to set the size of the next MI
        double sum = 0;

        for (double &rtt : rttsMi) {
            sum += rtt;
        }

        double avg = sum / rttsMi.size();

        lastMIavgRTT = avg;
    }

    // clear the gradient related vectors and insert the avg of this interval as first point of the next interval
    rttsMi.clear();
    rttsMiTimes.clear();

    conn->emit(rttGradientSignal, rttGradient);
    conn->emit(dupAcksSignal, dupAcks);

    obs.push_back(dupAcks);
    obs.push_back((double) rttGradient);
    obs.push_back((double) lastMiAction);
    obs.push_back(lastMIavgRTT);

    double isFastRecovery = 0;
    if (state->dupacks >= DUPTHRESH) {
        isFastRecovery = 1;
    }
    obs.push_back(isFastRecovery);

    double isAfterRto = 0;
    if (state->afterRto) {
        isAfterRto = 1;
    }
    obs.push_back(isAfterRto);

    dupAcks = 0;

}

void RLTcpNewReno::act() {
    if (isActive) {
        // instantiate empty vector that will be filled with the features of the observation
        std::vector<double> vec;

        // compute all features and store them in vec
        computeObservation(vec);

        // create an Observation from the features computed
        Observation obs(vec);

        // update state: handles states with more than one observation
        updateState(obs);

    if (state->dupacks < DUPTHRESH && !state->lossRecovery) {
            Query qry(stringId, rlState);
            owner->emit(querySignal, &qry);
        }

        EV_INFO << "Creating step instance....";

    }
}

void RLTcpNewReno::decisionMade(ActionType action) {
    if (!isnan(action)) {
        state->snd_cwnd = static_cast<uint32>(max((double) state->snd_mss, ceil(action * maxLearnWindow * (double) state->snd_mss)));
        conn->emit(actionSignal, action);
        conn->emit(cwndSignal, state->snd_cwnd);

        // pace connection
        if (isConnectionPaced) {
            ASSERT(lastMIavgRTT > 0);
            double paceD = lastMIavgRTT / ((double) state->snd_cwnd / (double) state->snd_mss);
            auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
            pacedConn->intersendingTime = paceD;
            pacedConn->paceValueVec.record(paceD);
        }
    }
    else {
        EV_ERROR << action << " value in decisionMade() function" << std::endl;
    }
}

void RLTcpNewReno::rttMeasurementComplete(simtime_t tSent, simtime_t tAcked) {
    //
    // Jacobson's algorithm for estimating RTT and adaptively setting RTO.
    //
    // Note: this implementation calculates in doubles. An impl. which uses
    // 500ms ticks is available from old tcpmodule.cc:calcRetransTimer().
    //

    // update smoothed RTT estimate (srtt) and variance (rttvar)
    const double g = 0.125;    // 1 / 8; (1 - alpha) where alpha == 7 / 8;

    //This is the value we are interested in and we want to store
    //*******************************************************
    simtime_t newRTT = tAcked - tSent;
    //*******************************************************

    rttsMi.push_back(newRTT.dbl());
    rttsMiTimes.push_back(simTime().dbl());

    simtime_t &srtt = state->srtt;
    simtime_t &rttvar = state->rttvar;

    simtime_t err = newRTT - srtt;

    srtt += g * err;
    rttvar += g * (fabs(err) - rttvar);

    // assign RTO (here: rexmit_timeout) a new value
    simtime_t rto = srtt + 4 * rttvar;

    if (rto > MAX_REXMIT_TIMEOUT)
        rto = MAX_REXMIT_TIMEOUT;
    else if (rto < MIN_REXMIT_TIMEOUT)
        rto = MIN_REXMIT_TIMEOUT;

    state->rexmit_timeout = rto;

    // record statistics
    EV_DETAIL << "Measured RTT=" << (newRTT * 1000) << "ms, updated SRTT=" << (srtt * 1000) << "ms, new RTO=" << (rto * 1000) << "ms\n";

    conn->emit(rttSignal, newRTT);
    conn->emit(srttSignal, srtt);
    conn->emit(rttvarSignal, rttvar);
    conn->emit(rtoSignal, rto);
}

void RLTcpNewReno::cleanup() {
}




ObsType RLTcpNewReno::getRLState() {
    return {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0};
}

RewardType RLTcpNewReno::getReward()
{
   
    return 0.0;

}


bool RLTcpNewReno::getDone()
{
    return 0;

}

}    // namespace learning

#endif