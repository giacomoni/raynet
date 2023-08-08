//
// Copyright (C) 2004-2005 OpenSim Ltd.
// Copyright (C) 2009 Thomas Reschka
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "RLTcpAlphaBeta.h"

#include <algorithm> // min,max

#include "inet/transportlayer/tcp/Tcp.h"

namespace inet {
namespace tcp {

Register_Class(RLTcpAlphaBeta);

RLTcpAlphaBeta::RLTcpAlphaBeta() :
        TcpTahoeRenoFamily(), state(
                (RLTcpAlphaBetaStateVariables*&) TcpAlgorithm::state) {
}

void RLTcpAlphaBeta::recalculateSlowStartThreshold() {
    // RFC 2581, page 4:
    // "When a TCP sender detects segment loss using the retransmission
    // timer, the value of ssthresh MUST be set to no more than the value
    // given in equation 3:
    //
    //   ssthresh = max (FlightSize / 2, 2*SMSS)            (3)
    //
    // As discussed above, FlightSize is the amount of outstanding data in
    // the network."

    // set ssthresh to flight size / 2, but at least 2 SMSS
    // (the formula below practically amounts to ssthresh = cwnd / 2 most of the time)

//     uint32_t current_window = state->snd_cwnd/state->snd_mss;
//     uint32_t target_window = state->beta*current_window;
//     uint32_t flight_size = std::min(target_window*state->snd_mss, state->snd_wnd); // FIXME - Does this formula computes the amount of outstanding data?
// //    uint32_t flight_size = state->snd_max - state->snd_una;
//     state->ssthresh = std::max(flight_size / 2, 2 * state->snd_mss);

    conn->emit(ssthreshSignal, state->ssthresh);
}

void RLTcpAlphaBeta::processRexmitTimer(TcpEventCode &event) {
    TcpTahoeRenoFamily::processRexmitTimer(event);

    if (event == TCP_E_ABORT)
        return;

    // After REXMIT timeout TCP Reno should start slow start with snd_cwnd = snd_mss.
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

    EV_INFO << "Begin Slow Start: resetting cwnd to " << state->snd_cwnd
                   << ", ssthresh=" << state->ssthresh << "\n";

    state->afterRto = true;

    conn->retransmitOneSegment(true);
}

void RLTcpAlphaBeta::receivedDataAck(uint32_t firstSeqAcked) {
    TcpTahoeRenoFamily::receivedDataAck(firstSeqAcked);

    //update Mearuments to be used when computing the state
    if(state->last_ack_received != 0){
        // Throughput in segments/s
        uint64_t throughput = 1/(simTime() - state->last_ack_received).dbl();
    }

    state->last_ack_received = simTime();

    if (state->snd_cwnd < state->ssthresh) {
        EV_INFO
                       << "cwnd <= ssthresh: Slow Start: increasing cwnd by one SMSS bytes to ";

        // perform Slow Start. RFC 2581: "During slow start, a TCP increments cwnd
        // by at most SMSS bytes for each ACK received that acknowledges new data."
        state->snd_cwnd += state->snd_mss;

        conn->emit(cwndSignal, state->snd_cwnd);
        conn->emit(ssthreshSignal, state->ssthresh);

        EV_INFO << "cwnd=" << state->snd_cwnd << "\n";
    } else {

        uint32_t cnt = state->snd_cwnd/(state->alpha*state->snd_mss);

        //* If credits accumulated at a higher w, apply them gently now. */
        if (state->cwnd_cnt >= cnt) {
            state->cwnd_cnt = 0;
            state->snd_cwnd += state->snd_mss;
        }

        state->cwnd_cnt++;
        if (state->cwnd_cnt >= cnt) {
            uint32_t delta = state->cwnd_cnt / cnt;

            state->cwnd_cnt -= delta * cnt;
            state->snd_cwnd += (delta*state->snd_mss);
        }

        conn->emit(cwndSignal, state->snd_cwnd);
        conn->emit(ssthreshSignal, state->ssthresh);

        //
        // Note: some implementations use extra additive constant mss / 8 here
        // which is known to be incorrect (RFC 2581 p5)
        //
        // Note 2: RFC 3465 (experimental) "Appropriate Byte Counting" (ABC)
        // would require maintaining a bytes_acked variable here which we don't do
        //

        EV_INFO
                       << "cwnd > ssthresh: Congestion Avoidance: increasing cwnd linearly, to "
                       << state->snd_cwnd << "\n";
    }

    // Check if recovery phase has ended
    if (state->lossRecovery && state->sack_enabled) {
        if (seqGE(state->snd_una, state->recoveryPoint)) {
            EV_INFO << "Loss Recovery terminated.\n";
            state->lossRecovery = false;
            conn->emit(lossRecoverySignal, 0);
        }
    }

    // Send data, either in the recovery mode or normal mode
    if (state->lossRecovery) {
        conn->setPipe();

        // RFC 3517, page 7: "(C) If cwnd - pipe >= 1 SMSS the sender SHOULD transmit one or more
        // segments as follows:"
        if (((int) (state->snd_cwnd / state->snd_mss)
                - (int) (state->pipe / (state->snd_mss - 12))) >= 1) // Note: Typecast needed to avoid prohibited transmissions
            conn->sendDataDuringLossRecoveryPhase(state->snd_cwnd);
    } else {
        sendData(false);
    }
}

void RLTcpAlphaBeta::receivedDuplicateAck() {
    TcpTahoeRenoFamily::receivedDuplicateAck();

    if (state->dupacks >= state->dupthresh) {
        if (!state->lossRecovery
                && (state->recoveryPoint == 0
                        || seqGE(state->snd_una, state->recoveryPoint))) {

            state->recoveryPoint = state->snd_max; // HighData = snd_max
            state->lossRecovery = true;
            EV_DETAIL << " recoveryPoint=" << state->recoveryPoint;
            conn->emit(lossRecoverySignal, 1);

            // enter Fast Recovery
            recalculateSlowStartThreshold();
            // "set cwnd to ssthresh plus 3 * SMSS." (RFC 2581)
            state->snd_cwnd = state->ssthresh + state->dupthresh*state->snd_mss;

            conn->emit(cwndSignal, state->snd_cwnd);

            EV_DETAIL << " set cwnd=" << state->snd_cwnd << ", ssthresh="
                             << state->ssthresh << "\n";

            // Fast Retransmission: retransmit missing segment without waiting
            // for the REXMIT timer to expire
            conn->retransmitOneSegment(false);
            conn->emit(highRxtSignal, state->highRxt);
        }

        conn->emit(cwndSignal, state->snd_cwnd);
        conn->emit(ssthreshSignal, state->ssthresh);

    }

    if (state->lossRecovery) {
        conn->setPipe();
        state->segments_lost++;

        if (((int) (state->snd_cwnd / state->snd_mss)
                - (int) (state->pipe / (state->snd_mss - 12))) >= 1) { // Note: Typecast needed to avoid prohibited transmissions
            conn->sendDataDuringLossRecoveryPhase(state->snd_cwnd);
        }
    }
}

} // namespace tcp
} // namespace inet

