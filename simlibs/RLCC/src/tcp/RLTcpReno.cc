//
// Copyright (C) 2020 Luca Giacomoni and George Parisis
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
#ifdef RLTCP

#include "RLTcpReno.h"

#define RTT_MULTIPLIER 1.7 // multiplier for the srtt when setting MI length
#define MI_MINIMUM_LENGTH 0.005 // default length of MI

//These value have  been copied from TcpBaseAlg because they are not visible from this file.
#define MIN_REXMIT_TIMEOUT 0.01
#define MAX_REXMIT_TIMEOUT 240

//Weight of the delay in reward function
#define DELAYWEIGHT 1000
//Weight of the throughput in reward function
#define THRWEIGHT 10

using namespace inet::tcp;
using namespace inet;

namespace learning {

Register_Class(RLTcpReno);

RLTcpReno::RLTcpReno() :
        TcpReno(), RLInterface() {
}

RLTcpReno::~RLTcpReno() {
    if (monitorInterval)
        delete cancelEvent(monitorInterval);

    getSimulation()->getSystemModule()->unsubscribe(stringId.c_str(), (cListener*) this);
    getSimulation()->getSystemModule()->unsubscribe("actionResponse", (cListener*) this);
}

void RLTcpReno::initialize() {
    int _stateSize;
    int _maxObsCount;
    double _ewmaWeight;

    // check underlying TcpConnection to decide if this a paced connection or not
    dynamic_cast<PacedTcpConnection*>(this->conn) ? isConnectionPaced = true : isConnectionPaced = false;

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
    TcpReno::initialize();

    // instanitate the EWMA filters
    interArrivalEwmaPtr = std::make_shared<Ewma>(_ewmaWeight);
    interSendEwmaPtr = std::make_shared<Ewma>(_ewmaWeight);
    rttEwmaPtr = std::make_shared<Ewma>(_ewmaWeight);

    dupAcks = 0;

    lastMIReward = -1;
    lastMIavgRTT = -1;

    throughputSignal = conn->registerSignal("throughput");
    actionSignal = conn->registerSignal("action");
    dupAcksSignal = conn->registerSignal("dupAcks");
    rttGradientSignal = conn->registerSignal("rttGradient");
    tickSignal = conn->registerSignal("tick");
    miQueueSizeSignal = conn->registerSignal("MIQueueSize");
}

void RLTcpReno::established(bool active) {
    TcpReno::established(active);

    if (active) {
        setStringId(conn->getLocalAddress().str());
        this->isActive = active;
        conn->emit(cwndSignal, state->snd_cwnd);
        conn->emit(dupAcksSignal, dupAcks);
    }
}

bool RLTcpReno::sendData(bool sendCommandInvoked) {
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

            double _action = (double) state->snd_cwnd / (double) (maxLearnWindow * state->snd_mss);
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

void RLTcpReno::receivedDataAck(uint32 firstSeqAcked) {

    //Since we measure RTT useing TS option, we want to make sure this is on.
    ASSERT(state->ts_enabled);
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

            if (isConnectionPaced) {
                auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
                double pace = lastMIavgRTT / ((double) state->snd_cwnd / (double) state->snd_mss);
                if (pace > 0)
                    pacedConn->changeIntersendingTime(pace);
            }

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

                // at this point, the reward assigned to the stEp should be -1 since no throughput has been assigned yet.
                ASSERT((*mi)->step->r == -1);
                // set the reward value of the step
                (*mi)->step->r = THRWEIGHT * throughput - DELAYWEIGHT * (*mi)->avgRtt;
                cout << "Computed reward of: " << (*mi)->step->r << ". Sending to replayBuffer. " << std::endl;
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
    }
    else {
        TcpReno::receivedDataAck(firstSeqAcked);
    }
}

void RLTcpReno::receivedDuplicateAck() {
    if (isActive) {

        dupAcks++;

        TcpTahoeRenoFamily::receivedDuplicateAck();

        if (state->dupacks == DUPTHRESH) {

            if (state->sack_enabled) {
                // RFC 3517, page 6: "When a TCP sender receives the duplicate ACK corresponding to
                // DupThresh ACKs, the scoreboard MUST be updated with the new SACK
                // information (via Update ()).  If no previous loss event has occurred
                // on the connection or the cumulative acknowledgment point is beyond
                // the last value of RecoveryPoint, a loss recovery phase SHOULD be
                // initiated, per the fast retransmit algorithm outlined in [RFC2581].
                // The following steps MUST be taken:
                //
                // (1) RecoveryPoint = HighData
                //
                // When the TCP sender receives a cumulative ACK for this data octet
                // the loss recovery phase is terminated."

                // RFC 3517, page 8: "If an RTO occurs during loss recovery as specified in this document,
                // RecoveryPoint MUST be set to HighData.  Further, the new value of
                // RecoveryPoint MUST be preserved and the loss recovery algorithm
                // outlined in this document MUST be terminated.  In addition, a new
                // recovery phase (as described in section 5) MUST NOT be initiated
                // until HighACK is greater than or equal to the new value of
                // RecoveryPoint."
                if (state->recoveryPoint == 0 || seqGE(state->snd_una, state->recoveryPoint)) {    // HighACK = snd_una
                    state->recoveryPoint = state->snd_max;    // HighData = snd_max
                    state->lossRecovery = true;
                    EV_DETAIL << " recoveryPoint=" << state->recoveryPoint;
                }
            }

            // enter Fast Recovery
            recalculateSlowStartThreshold();
            // "set cwnd to ssthresh plus 3 * SMSS." (RFC 2581)
            state->snd_cwnd = state->ssthresh + 3 * state->snd_mss;    // 20051129 (1)

            if (isConnectionPaced) {
                auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
                double pace = lastMIavgRTT / ((double) state->snd_cwnd / (double) state->snd_mss);
                if (pace > 0)
                    pacedConn->changeIntersendingTime(pace);
            }

            conn->emit(cwndSignal, state->snd_cwnd);

            EV_DETAIL << " set cwnd=" << state->snd_cwnd << ", ssthresh=" << state->ssthresh << "\n";

            // Fast Retransmission: retransmit missing segment without waiting
            // for the REXMIT timer to expire
            conn->retransmitOneSegment(false);
            // Do not restart REXMIT timer.
            // Note: Restart of REXMIT timer on retransmission is not part of RFC 2581, however optional in RFC 3517 if sent during recovery.
            // Resetting the REXMIT timer is discussed in RFC 2582/3782 (NewReno) and RFC 2988.

            if (state->sack_enabled) {
                // RFC 3517, page 7: "(4) Run SetPipe ()
                //
                // Set a "pipe" variable  to the number of outstanding octets
                // currently "in the pipe"; this is the data which has been sent by
                // the TCP sender but for which no cumulative or selective
                // acknowledgment has been received and the data has not been
                // determined to have been dropped in the network.  It is assumed
                // that the data is still traversing the network path."
                conn->setPipe();
                // RFC 3517, page 7: "(5) In order to take advantage of potential additional available
                // cwnd, proceed to step (C) below."
                if (state->lossRecovery) {
                    // RFC 3517, page 9: "Therefore we give implementers the latitude to use the standard
                    // [RFC2988] style RTO management or, optionally, a more careful variant
                    // that re-arms the RTO timer on each retransmission that is sent during
                    // recovery MAY be used.  This provides a more conservative timer than
                    // specified in [RFC2988], and so may not always be an attractive
                    // alternative.  However, in some cases it may prevent needless
                    // retransmissions, go-back-N transmission and further reduction of the
                    // congestion window."
                    // Note: Restart of REXMIT timer on retransmission is not part of RFC 2581, however optional in RFC 3517 if sent during recovery.
                    EV_INFO << "Retransmission sent during recovery, restarting REXMIT timer.\n";
                    restartRexmitTimer();

                    // RFC 3517, page 7: "(C) If cwnd - pipe >= 1 SMSS the sender SHOULD transmit one or more
                    // segments as follows:"
                    if (((int) state->snd_cwnd - (int) state->pipe) >= (int) state->snd_mss) // Note: Typecast needed to avoid prohibited transmissions
                        conn->sendDataDuringLossRecoveryPhase(state->snd_cwnd);
                }
            }

            // try to transmit new segments (RFC 2581)
            sendData(false);
        }
        else if (state->dupacks > DUPTHRESH) {
            //
            // Reno: For each additional duplicate ACK received, increment cwnd by SMSS.
            // This artificially inflates the congestion window in order to reflect the
            // additional segment that has left the network
            //
            state->snd_cwnd += state->snd_mss;

            if (isConnectionPaced) {
                auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
                double pace = lastMIavgRTT / ((double) state->snd_cwnd / (double) state->snd_mss);
                if (pace > 0)
                    pacedConn->changeIntersendingTime(pace);
            }

            EV_DETAIL << "Reno on dupAcks > DUPTHRESH(=3): Fast Recovery: inflating cwnd by SMSS, new cwnd=" << state->snd_cwnd << "\n";

            conn->emit(cwndSignal, state->snd_cwnd);

            // Note: Steps (A) - (C) of RFC 3517, page 7 ("Once a TCP is in the loss recovery phase the following procedure MUST be used for each arriving ACK")
            // should not be used here!

            // RFC 3517, pages 7 and 8: "5.1 Retransmission Timeouts
            // (...)
            // If there are segments missing from the receiver's buffer following
            // processing of the retransmitted segment, the corresponding ACK will
            // contain SACK information.  In this case, a TCP sender SHOULD use this
            // SACK information when determining what data should be sent in each
            // segment of the slow start.  The exact algorithm for this selection is
            // not specified in this document (specifically NextSeg () is
            // inappropriate during slow start after an RTO).  A relatively
            // straightforward approach to "filling in" the sequence space reported
            // as missing should be a reasonable approach."
            sendData(false);
        }
    }
}

void RLTcpReno::processTimer(cMessage *timer, TcpEventCode &event) {
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

void RLTcpReno::processRexmitTimer(TcpEventCode &event) {
    if (isActive) {
        TcpTahoeRenoFamily::processRexmitTimer(event);

        if (event == TCP_E_ABORT)
            return;

        recalculateSlowStartThreshold();
        state->snd_cwnd = state->snd_mss;
        state->afterRto = true;

        if (isConnectionPaced) {
            auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
            double pace = lastMIavgRTT / ((double) state->snd_cwnd / (double) state->snd_mss);
            if (pace > 0)
                pacedConn->changeIntersendingTime(pace);
        }

        conn->retransmitOneSegment(true);
    }
    else {
        TcpReno::processRexmitTimer(event);
    }
}

void RLTcpReno::processMonitorIntervalTimer(TcpEventCode &event) {
    if (miHandler.currentMi->isFirstSegmentSent)
        tick();
    else {
        EV_INFO << "Not a single packet sent yet for MI " << miHandler.currentMi->miNumber << ". Deferring ticking to when a packet is sent." << std::endl;
        miHandler.currentMi->deferredTicking = true;
    }
}

void RLTcpReno::tick() {
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
        miHandler.currentMi->step->r = THRWEIGHT * throughput - DELAYWEIGHT * miHandler.currentMi->avgRtt;
        cout << "Computed reward (during tick) of: " << miHandler.currentMi->step->r << ". Sending to replayBuffer. " << std::endl;
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

void RLTcpReno::computeObservation(std::vector<double> &obs) {
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

        //set the avf RTT of the MI too be used when the reawrd is computed later on time, that is when throughput can be calculated
        miHandler.currentMi->avgRtt = avg;
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

void RLTcpReno::act() {
    if (isActive) {
        // instantiate empty vector that will be filled with the features of the observation
        std::vector<double> vec;

        // compute all features and store them in vec
        computeObservation(vec);

        // create an Observation from the features computed
        Observation obs(vec);

        // update state: handles states with more than one observation
        updateState(obs);

        if (state->dupacks < DUPTHRESH) {
            Query qry(stringId, rlState);
            owner->emit(querySignal, &qry);
        }

        EV_INFO << "Creating step instance....";

    }
}

void RLTcpReno::decisionMade(ActionType action) {
    if (!isnan(action)) {
        state->snd_cwnd = static_cast<uint32>(max((double) state->snd_mss, ceil(action * maxLearnWindow * (double) state->snd_mss)));
        conn->emit(actionSignal, action);
        conn->emit(cwndSignal, state->snd_cwnd);

        if (isConnectionPaced) {
            auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
            double pace = lastMIavgRTT / ((double) state->snd_cwnd / (double) state->snd_mss);
            if (pace > 0)
                pacedConn->changeIntersendingTime(pace);
        }
    }
    else {
        EV_ERROR << action << " value in decisionMade() function" << std::endl;
    }
}

void RLTcpReno::rttMeasurementComplete(simtime_t tSent, simtime_t tAcked) {
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

void RLTcpReno::cleanup() {
}

    


ObsType RLTcpReno::getRLState() {
    return {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0, 0};
  
}

RewardType RLTcpReno::getReward()
{
   
    return 0.0;

}

bool RLTcpReno::getDone()
{
    return 0;

}


}    // namespace learning

#endif