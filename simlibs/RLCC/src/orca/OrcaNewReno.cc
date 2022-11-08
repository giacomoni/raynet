#ifdef ORCA
#include "OrcaNewReno.h"


#define MIN_REXMIT_TIMEOUT 0.01
#define MAX_REXMIT_TIMEOUT 240

using namespace inet::tcp;
using namespace inet;

namespace learning {

Register_Class(OrcaNewReno);

OrcaNewReno::OrcaNewReno() : TcpNewReno(), RLInterface() {
    
}



void OrcaNewReno::initialize() {
    //cout << "Initialise TCP newReno" << endl;
    TcpNewReno::initialize();
    lastReportTime = SIMTIME_ZERO;
    firstPacketSentSeqNum = state->snd_una;
    seqPacketLoss = state->snd_una;

}


void OrcaNewReno::initRLAgent(){
    //cout << "Initialise for RL" << endl;
    // provide the RLInterface with a cComponent API (to use signaling functionality)
    this->setOwner((cComponent*) conn->getTcpMain());
    RLInterface::initialise();


    // Generate ID for this agent
    std::string s(this->conn->getFullPath());
    std::string token = s.substr(s.find("-") + 1);
    this->setStringId(token);
    //cout << "string id is " << stringId <<endl;
    
  

    start_snd_una = state->snd_una;
    start_time = simTime().dbl();




    // ----- Per Step Variables ----------

    // firstPacketSentSeqNum = state->snd_una;
    // seqPacketLoss = state->snd_una;
    // timeFirstPacketSent = simTime();
    maxValueOfThroughputSoFar = computeThroughput();
    throughput = 0;
    firstPacketSentSeqNum = state->snd_una;
    averageLossRateOfPackets = 0;
    numOfLostPackets = 0;
    numOfPacketsSent = state->snd_max; // start of with the max seq number sent 

    averageDelayOfPackets = 0;

    numOfValidAckPackets = 0;
    
    timeBetweenLastReportAndCurrentReport = 0.005;//computeInitialAverageDelayOfPackets(); // time MTP for reset will last average RTT of packets recieved prior to initialising(during SS) or 20ms(this should never be the case).
    timeForNextMTP = timeBetweenLastReportAndCurrentReport; // The first step should have this MTP length if no acks are recieved. This value should not be reset.
    RTTMI.clear();
    // previousStepTimestamp = simTime();
    // numberOfTrimmedPacketsStep = 0;
    // numberOfTrimmedBytesStep = 0;
    // sRttStep = SIMTIME_ZERO;
    // minRttStep = SIMTIME_ZERO;
    // goodputStep = 0; //Updated everytime we receive a new data packet.
    // rttvarStep = SIMTIME_ZERO;
    // numberOfDataPacketsStep = 0;
    // numberOfRecoveredDataPacketsStep = 0;



    // ----- Per Step Variables ----------

    smoothRTTOfPacketsSoFar = 0;
    currentCongestionWindow = 0;
    minValueOfPacketDelaySoFar = 100000000; //TODO: change to zero once isReset is fixed.
    // maxValueOfThroughputSoFar = 0;

    recievedAckslastMTP = false;
    // ----- Per flow variable
    // sRtt = 0;
    // minRtt = SIMTIME_ZERO;
    // goodput = 0;
    // latestRtt = SIMTIME_ZERO;
    // rttvar = SIMTIME_ZERO;
    rlInitialised = true;


    cObject* simtime = new cSimTime(SimTime(timeBetweenLastReportAndCurrentReport));
    conn -> emit(this->registerSig, token.c_str(), simtime); //register the agent with stepper and broker
    cout << ":::::::::::::::::::::::::::::::RL agent init "<< endl;
}


void OrcaNewReno::established(bool active)
{
    TcpNewReno::established(active);
}

bool OrcaNewReno::sendData(bool sendCommandInvoked) {
    //cout << "_in sendData!" <<endl;
    //cout << "state_scwnd: " << state->snd_cwnd <<endl;
    bool b;
    uint32_t oldSndMax, newSndMax;

    oldSndMax = state->snd_max;
    b = TcpBaseAlg::sendData(sendCommandInvoked);
    newSndMax = state->snd_max;
 
   
    EV_INFO << "Sent " << newSndMax - oldSndMax << " bytes" << std::endl; //max seq number(sequence number to be sent next) - previous max sequence number sent = how many bytes sent

    //first unacked segement sent
    if(state -> snd_una ==1)
    {
        // ssssssssscout << state->snd_una <<  endl;
        segmentSentAtMIStart = 1;
        previousUnacked = 1;
        previousUnackedTime = simTime();


        //start_snd_una = state->snd_una;
        //start_time = simTime().dbl();

    }

    return b;
}

void OrcaNewReno::processRexmitTimer(TcpEventCode &event)
{
    TcpNewReno::processRexmitTimer(event);
    if (rlInitialised)
    {
        numOfLostPackets += 1; // obs. Triggered when a packet has been lost by timeout, so increment obs by one as here the lost packet is being retransmitted. 
    }
}

void OrcaNewReno::receivedDataAck(uint32_t firstSeqAcked)
{
  //cout << "in recieved ack" << endl;


    TcpTahoeRenoFamily::receivedDataAck(firstSeqAcked);
    //cout << "after in recieved ack" << endl;
    // RFC 3782, page 5:
    // "5) When an ACK arrives that acknowledges new data, this ACK could be
    // the acknowledgment elicited by the retransmission from step 2, or
    // elicited by a later retransmission.
    //
    // Full acknowledgements:
    // If this ACK acknowledges all of the data up to and including
    // "recover", then the ACK acknowledges all the intermediate
    // segments sent between the original transmission of the lost
    // segment and the receipt of the third duplicate ACK.  Set cwnd to
    // either (1) min (ssthresh, FlightSize + SMSS) or (2) ssthresh,
    // where ssthresh is the value set in step 1; this is termed
    // "deflating" the window.  (We note that "FlightSize" in step 1
    // referred to the amount of data outstanding in step 1, when Fast
    // Recovery was entered, while "FlightSize" in step 5 refers to the
    // amount of data outstanding in step 5, when Fast Recovery is
    // exited.)  If the second option is selected, the implementation is
    // encouraged to take measures to avoid a possible burst of data, in
    // case the amount of data outstanding in the network is much less
    // than the new congestion window allows.  A simple mechanism is to
    // limit the number of data packets that can be sent in response to
    // a single acknowledgement; this is known as "maxburst_" in the NS
    // simulator.  Exit the Fast Recovery procedure."

    if (state->lossRecovery)
    {
        if (seqGE(state->snd_una - 1, state->recover))
        {
            // Exit Fast Recovery: deflating cwnd
            //
            // option (1): set cwnd to min (ssthresh, FlightSize + SMSS)
            uint32_t flight_size = state->snd_max - state->snd_una;
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

            if (rlInitialised)
            {
                numOfValidAckPackets += 1; // obs
            }
        }
        else
        {
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
            //cout << "retransmit seg" << endl;
            if (rlInitialised)
            {
                numOfLostPackets += 1; // obs. A partial ack has been recieved. So another packet was lost in the same window when in loss recovery. This has been retranmitted. Increment obs by one.
            }

            // deflate cwnd by amount of new data acknowledged by cumulative acknowledgement field
            state->snd_cwnd -= state->snd_una - firstSeqAcked;

            conn->emit(cwndSignal, state->snd_cwnd);

            EV_INFO << "Fast Recovery: deflating cwnd by amount of new data acknowledged, new cwnd=" << state->snd_cwnd << "\n";

            // if the partial ACK acknowledges at least one SMSS of new data, then add back SMSS bytes to the cwnd
            if (state->snd_una - firstSeqAcked >= state->snd_mss)
            {
                state->snd_cwnd += state->snd_mss;

                conn->emit(cwndSignal, state->snd_cwnd);

                EV_DETAIL << "Fast Recovery: inflating cwnd by SMSS, new cwnd=" << state->snd_cwnd << "\n";
            }

            // try to send a new segment if permitted by the new value of cwnd
            sendData(false);

            // reset REXMIT timer for the first partial ACK that arrives during Fast Recovery
            if (state->lossRecovery)
            {
                if (!state->firstPartialACK)
                {
                    state->firstPartialACK = true;
                    EV_DETAIL << "First partial ACK arrived during recovery, restarting REXMIT timer.\n";
                    restartRexmitTimer();
                }
            }
        }
    }
    else
    {
        
        //
        // Perform slow start and congestion avoidance.
        //
        if (state->snd_cwnd < state->ssthresh)
        {
            EV_DETAIL << "cwnd <= ssthresh: Slow Start: increasing cwnd by SMSS bytes to ";

            // perform Slow Start. RFC 2581: "During slow start, a TCP increments cwnd
            // by at most SMSS bytes for each ACK received that acknowledges new data."
            state->snd_cwnd += state->snd_mss;

            // Note: we could increase cwnd based on the number of bytes being
            // acknowledged by each arriving ACK, rather than by the number of ACKs
            // that arrive. This is called "Appropriate Byte Counting" (ABC) and is
            // described in RFC 3465. This RFC is experimental and probably not
            // implemented in real-life TCPs, hence it's commented out. Also, the ABC
            // RFC would require other modifications as well in addition to the
            // two lines below.
            //
            // int bytesAcked = state->snd_una - firstSeqAcked;
            // state->snd_cwnd += bytesAcked * state->snd_mss;

            conn->emit(cwndSignal, state->snd_cwnd);

            EV_DETAIL << "cwnd=" << state->snd_cwnd << "\n";
            //cout << "thres is: " << state->ssthresh << " and congestion window is: "<< state->snd_cwnd <<endl;
        }
        else
        {
            /**
             * Start RL the first time newReno enters CA.
             * 
             */
            //cout << "CongestionAvoidance" << endl; 
            if(!rlInitialised){
                initRLAgent();
                rlInitialised = true;
            }
            // perform Congestion Avoidance (RFC 2581)
            uint32_t incr = state->snd_mss * state->snd_mss / state->snd_cwnd;

            if (incr == 0)
                incr = 1;

            state->snd_cwnd += incr;

            conn->emit(cwndSignal, state->snd_cwnd);

            //
            // Note: some implementations use extra additive constant mss / 8 here
            // which is known to be incorrect (RFC 2581 p5)
            //
            // Note 2: RFC 3465 (experimental) "Appropriate Byte Counting" (ABC)
            // would require maintaining a bytes_acked variable here which we don't do
            //

            EV_DETAIL << "cwnd > ssthresh: Congestion Avoidance: increasing cwnd linearly, to " << state->snd_cwnd << "\n";
        }

        // RFC 3782, page 13:
        // "When not in Fast Recovery, the value of the state variable "recover"
        // should be pulled along with the value of the state variable for
        // acknowledgments (typically, "snd_una") so that, when large amounts of
        // data have been sent and acked, the sequence space does not wrap and
        // falsely indicate that Fast Recovery should not be entered (Section 3,
        // step 1, last paragraph)."
        state->recover = (state->snd_una - 2);

        if (rlInitialised)
        {
            numOfValidAckPackets += 1; // obs
        }
    }
    
    // // how much data has been sent to the reciever
    bytesAcked = state->snd_una - firstSeqAcked;


    //cout changes in snd_una around here. if we have got some acked data then send unacknowledged will update. If it has updated then set new time
    
    if(state -> snd_una > previousUnacked)
    {
        currentUnackedTime = simTime();

    }

    EV_INFO << "hi" << endl;
    //cout <<  "leaving recack" << endl;
    sendData(false);
    
    //breaks
    // if(state->snd_una == state->snd_max){
    //     this->done = true;
    // }


}

void OrcaNewReno::receivedDuplicateAck()
{
    //cout << "in recieved dup acks" << endl;

    TcpTahoeRenoFamily::receivedDuplicateAck();

    if (state->dupacks == state->dupthresh)
    { // DUPTHRESH = 3
        if (!state->lossRecovery)
        {
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
            if (state->snd_una - 1 > state->recover)
            {
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
                //cout << "retransmit seg" << endl;
                if (rlInitialised)
                {
                    numOfLostPackets += 1; // obs. Lost a packet due to recieving 3 DUP ACKS. Increment obs by one.
                }

                // RFC 3782, page 5:
                // "4) Fast Recovery, continued:
                // Transmit a segment, if allowed by the new value of cwnd and the
                // receiver's advertised window."
                sendData(false);
            }
            else
            {
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
    else if (state->dupacks > state->dupthresh)
    { // DUPTHRESH = 3
        if (state->lossRecovery)
        {
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
/**
 * Get the RTT for each ack recieved 
 */
void OrcaNewReno::rttMeasurementComplete(simtime_t tSent, simtime_t tAcked) {
    //cout << "in rttMeasurementComplete" << endl;

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
    //cout << "__________________________________________sender current unaacked seq num: " << state ->snd_una << endl;

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

    RTTMI.push_back(newRTT.dbl()); // obs
    if (rlInitialised)
    {
        smoothRTTOfPacketsSoFar = srtt.dbl(); // obs
        // cout << "seqPacketloss before is: " << seqPacketLoss << endl;
    }
    // throughput obs
    if (seqPacketLoss == (state->snd_una - state->snd_mss) + 12)
    {
        // cout << "acked in seq" <<endl;
        // packet loss. recieved ack for seqPacketLoss. waiting for ack for next seg with seq num state->snd_una
        times[(state->snd_una - state->snd_mss) + 12] = {tSent.dbl(), tAcked.dbl()}; //+12 needed to accommodate timestamp header space
        seqPacketLoss = state->snd_una;
    }
    else
    {
        // cout << "acked not in seq" <<endl;
        times[seqPacketLoss] = {tSent.dbl(), tAcked.dbl()}; //+12 needed to accommodate timestamp header space
        seqPacketLoss = state->snd_una;
            seqPacketLoss = state->snd_una; 
        seqPacketLoss = state->snd_una;
            seqPacketLoss = state->snd_una; 
        seqPacketLoss = state->snd_una;
            seqPacketLoss = state->snd_una; 
        seqPacketLoss = state->snd_una;
            seqPacketLoss = state->snd_una; 
        seqPacketLoss = state->snd_una;
            seqPacketLoss = state->snd_una; 
        seqPacketLoss = state->snd_una;
            seqPacketLoss = state->snd_una; 
        seqPacketLoss = state->snd_una;
    }
    // cout << "seqPacketloss after is: " << seqPacketLoss << endl;

    // record statistics
    EV_DETAIL << "Measured RTT=" << (newRTT * 1000) << "ms, updated SRTT=" << (srtt * 1000) << "ms, new RTO=" << (rto * 1000) << "ms\n";

    conn->emit(rttSignal, newRTT);
    conn->emit(srttSignal, srtt);
    conn->emit(rttvarSignal, rttvar);
    conn->emit(rtoSignal, rto);
    
}


/**
 * apply actions recieved from policy to the agent
 */
void OrcaNewReno::decisionMade(float action)
{
    //cout << "action is: " << action -2 << endl;
    float action_changed = action;

    cString *c_name = new cString(stringId);
    conn->emit(modifyStepSizeSig, 0.005, c_name);
    if (recievedAckslastMTP)
    {
        //cout << "_in OrcaNewReno decisionmade and action is: " << action_changed << endl;
        //cout << "previous cwnd is: " << state->snd_cwnd << endl;
        state->snd_cwnd = max((double) 2*state->snd_mss,state->snd_cwnd * pow(2, action_changed));
        conn->emit(cwndSignal, state->snd_cwnd);
        //cout << "new cwnd is: " << state->snd_cwnd << endl;
        double old_snd_nxt = state->snd_nxt;
        actionDistribution[action_changed] += 1;

        //after having the cwnd, pace rate is calculated as cwnd/sRTT
        auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
        pacedConn->changeIntersendingTime((double)smoothRTTOfPacketsSoFar/ ((double)state->snd_cwnd/(double)state->snd_mss));
        

        // sendData(false);
        //cout << "send next changed from: " << old_snd_nxt << " to " << state->snd_nxt << endl;
    }
    else
    {
        //cout << "_in OrcaNewReno decisionmade and action is: " << action_changed << endl;
        //cout << "previous cwnd is: " << state->snd_cwnd << endl;
        //cout << "new cwnd is: " << state->snd_cwnd << endl;
        //cout << "no ack recieved at the last MTP so action not applied." << endl;
    }

}

//-------------------------------------------------------- LEGACY CODE ----------------------------------------------------------------------------------------------------------------------------------------------------



//TODO: use cmdenv trace for debugging
/**
 * @return the observations from the 
 */
ObsType OrcaNewReno::getRLState()
{
    
    ObsType  rlState = {0,0,0,0,0,0,0,0,0,0};
    return rlState;
}

//Todo: unacked MI
/** 
 * @return the reward of the step
 */
RewardType OrcaNewReno::getReward()
{
    double reward;
    //cout << throughput << " " << averageLossRateOfPackets << " " << maxValueOfThroughputSoFar << " " << minValueOfPacketDelaySoFar << " " << averageDelayOfPackets << endl;
    if(averageDelayOfPackets != 0 && maxValueOfThroughputSoFar !=0)
    {
        if(averageDelayOfPackets <= 1.25 * minValueOfPacketDelaySoFar){
        reward = ((throughput -  (5*averageLossRateOfPackets*1500))/maxValueOfThroughputSoFar) * 1;
        }
        else{
            reward = ((throughput -  (5*averageLossRateOfPackets*1500))/maxValueOfThroughputSoFar) * (minValueOfPacketDelaySoFar/averageDelayOfPackets);
        }
        
    }
    else{
        reward = 0;

    }
    return reward;
}

/**
 * 
 * @return if all the data has been sent and recieved in the step
 */
bool OrcaNewReno::getDone()
{
    this->done = state->snd_una == state->snd_max;
    if(this->done == true){

        //  for (map<double, int>::iterator itr = actionDistribution.begin(); itr != actionDistribution.end(); ++itr)
        // {
        //     cout << " -----------------------------------------------------------------------------------------------------------------------action space distribution, action: " << itr->first << " frequency: " << itr->second << endl;
        // }

        cout << "-------------------------------------------------------------------------------------TCP flow throughput agent RL(bits/sec): "<< ((state->snd_una - start_snd_una) * 8)/(simTime().dbl() - start_time) << " start byte sent is: "<< start_snd_una<< " last byte sent is: "<< state->snd_una << " time first byte sent is: " << start_time << " time last byte recieved is: " << simTime().dbl()<<endl;
    }
    return this->done;
}




//-------------------------------------------------------- LEGACY CODE ----------------------------------------------------------------------------------------------------------------------------------------------------

void OrcaNewReno::cleanup()
{

}

/**
 * return the observations for the step
 */
ObsType OrcaNewReno::computeObservation()
{
    recievedAckslastMTP = RTTMI.size(); // false if no acks were recieved this MTP
    // cout << "::::num of acks rec: " << RTTMI.size() << endl;
    double reportTimeDiff;
    if (lastReportTime != SIMTIME_ZERO)
        reportTimeDiff = (simTime() - lastReportTime).dbl();
    else
        reportTimeDiff = 0;
    
    // lastReportTime = simTime();

    if(recievedAckslastMTP)
        lastReportTime = simTime();

    // cout << "::::::;;packets in cwnd " << state->snd_cwnd/state->snd_mss << endl;
    auto pacedConn = check_and_cast<PacedTcpConnection*>(this->conn);
    
    // cout << ":::::::::::::: intersending time: " << pacedConn->intersendingTime << endl;
    // cout << ":::::::::::::: sRTT " <<  smoothRTTOfPacketsSoFar << endl;
    // cout << ":::::::::::::: qsize " << pacedConn->packetQueue.size() << endl;
    // cout << "\n\n\n\n" <<endl;
    return {computeThroughput()/1000000, 
    computeAverageLossRateOfPackets(), 
    computeAverageDelayOfPackets(), 
    numOfValidAckPackets,
    reportTimeDiff, 
    smoothRTTOfPacketsSoFar, 
    computeCurrentCongestionWindow()/10000.0, 
    
    computeMaxValueOfThroughputSoFar()/1000000, 
    computeMinValueOfPacketDelaySoFar(),
    //extra obs not used by the agent:
    float(state->snd_cwnd/state->snd_mss)
    
    };
}

void OrcaNewReno::resetStepVariables()
{
    //cout << "reset step variables" << endl;

    throughput = 0;
    firstPacketSentSeqNum = state->snd_una; //get seq number of first packet currently unacked.

    times.clear();




    averageLossRateOfPackets = 0;
    numOfLostPackets = 0;
    numOfPacketsSent = state->snd_max;


    averageDelayOfPackets = 0;
    RTTMI.clear(); //empty vector to only store the RTT for the next step.

    numOfValidAckPackets = 0;

    timeBetweenLastReportAndCurrentReport = 0;

  

}

RewardType OrcaNewReno::computeReward()
{
    return 0;
}

/**
 * compute the average delay of packets.
 * This is called from compute observation per step.
 */
double OrcaNewReno::computeAverageDelayOfPackets()
{
    if (RTTMI.size() != 0) // recieved new ack packets this step. RTTs are measured to 3 decimal places due to using timestamps. This caused an issue when RTTs came back as ver low values of zero. So set delay in config as at least 1ms. 
    {
        // calculate sum of RTTs in this step
        double RTTSum = 0;
        for (double value : RTTMI)
        {
            RTTSum += value;
        }
        averageDelayOfPackets = RTTSum / RTTMI.size(); 
    }
    else
    { 
        // no acks were recieved in this step and average delay is set to zero.todo: MTP with no acks
        averageDelayOfPackets = timeForNextMTP;
    }
    
    return averageDelayOfPackets;
}

double OrcaNewReno::computeAverageLossRateOfPackets()
{
    //cout << "num of lost packets is: " << numOfLostPackets << endl;
    if(RTTMI.size() != 0 || numOfLostPackets != 0){
        //cout << "averageLossrateofpackets is: " << averageLossRateOfPackets << " numoflostpackets is: " << numOfLostPackets << " state->snd_max " << state->snd_max << " numOfPacketsSent is: " << numOfPacketsSent << endl;
        averageLossRateOfPackets = numOfLostPackets / (numOfLostPackets + RTTMI.size()); // max seq number sent at end of step - max seq number sent at start of step = number of packets sent(TODO: divide by mss?)
    }else{
        averageLossRateOfPackets = 1;
    }

    
    return averageLossRateOfPackets;
}

double OrcaNewReno::computeTimeBetweenLastReportAndCurrentReport()
{

    if(!isReset){
        timeBetweenLastReportAndCurrentReport = timeForNextMTP; // if it is a step, the MTP length has been calculated in the previous MTP in the varaible timeForNextMTP
    }
    
    computeTimeForNextMTP();//compute min RTT during current MTP to be set for next MTP

    return timeBetweenLastReportAndCurrentReport;
}

void OrcaNewReno::computeTimeForNextMTP()
{
    if (RTTMI.size() != 0) // recieved new ack packets this step.
    {
       
        // print out the RTT's
        // std::cout << "RTTMI = { ";
        // for (double n : RTTMI)
        // {
        //     std::cout << n << ", ";
        // }
        // std::cout << "}; \n";

        double minRTT = RTTMI[0];
        for (double value : RTTMI)
        {
            if(value < minRTT){ //TODO: can a RTT be 0?
                minRTT = value; 
            }
        }

        if (minRTT == 0)
        {
            timeForNextMTP = timeBetweenLastReportAndCurrentReport;
        }
        else
        {
            timeForNextMTP = minRTT;
        }
    }
    else
    { 
        // no acks were recieved in this step and time for next MTP is set to the time for this current MTP.
        timeForNextMTP = timeBetweenLastReportAndCurrentReport;
    }
    
}

double OrcaNewReno::computeCurrentCongestionWindow()
{
    currentCongestionWindow = state->snd_cwnd;
    return currentCongestionWindow;
}

/**
 * TODO: change this method and no longer init minValueOfPacketDelaySoFar to zero once isReset is fixed.
 */
double OrcaNewReno::computeMinValueOfPacketDelaySoFar()
{
    // if (!isReset)
    // {   
        
    //     double delay = computeAverageDelayOfPackets();
    //     if (delay < minValueOfPacketDelaySoFar)
    //     {
    //         minValueOfPacketDelaySoFar = delay;
    //     }
    // }
    // else
    // {
    //     minValueOfPacketDelaySoFar = computeAverageDelayOfPackets();
    // }
    double delay = computeAverageDelayOfPackets();
    if(delay < minValueOfPacketDelaySoFar)
    {
        minValueOfPacketDelaySoFar = delay;
    }
    return minValueOfPacketDelaySoFar;
}

/**
 * each mtp calculate the number of packets acked / time it took for those packets to be acked.
 */
double OrcaNewReno::computeThroughput()
{
    //cout << "in throughput" << endl;
    //cout << times.size() << endl;
    if (RTTMI.size() != 0)
    {
        // print out all values
        // for (map<double, vector<double>>::iterator itr = times.begin(); itr != times.end(); ++itr)
        // {
        //     cout << "packet seq num: " << itr->first << " packet sent at: " << itr->second[0] << " packet recieved at: " << itr->second[1] << endl;
        // }

        // bytes acknowledged is last segment acked - first segment sent
        double bytesAcked = state->snd_una - firstPacketSentSeqNum;
        //cout << "hit1" << endl;

        map<double, vector<double>>::iterator itr = times.end();
        --itr;
        // cout << "Here is the last key/value pair in m: "
        //      << itr->first << ", " << itr->second[0] << "\n\n";

        double lastPacketAckRecievedTimeStep = itr->second[1];

        //cout << "hit1" << endl;

        map<double, vector<double>>::iterator itr2 = times.begin();
        // itr2;
        // cout << "Here is the first key/value pair in m: "
        //      << itr2->first << ", " << itr2->second[0] << "\n\n";

        double firstPacketSentTimeStep = itr2->second[0];

        double duration = lastPacketAckRecievedTimeStep - firstPacketSentTimeStep;

        if(duration == 0){ //TODO: times are really small?
            throughput = 0;
        }
        else{
             throughput = bytesAcked / duration;
        }

        //cout << "check: " << duration << endl;
    }
    else{
        throughput = 0;
    }

    return throughput;
}

double OrcaNewReno::computeMaxValueOfThroughputSoFar()
{
 
    double thr = computeThroughput();

    if(thr > maxValueOfThroughputSoFar)
    {
        maxValueOfThroughputSoFar = thr;
    }
    return maxValueOfThroughputSoFar;
}

double OrcaNewReno::computeInitialAverageDelayOfPackets()
{
    double initialAverageDelayOfPackets;
    if (RTTMI.size() != 0) // recieved new ack packets this step. RTTs are measured to 3 decimal places due to using timestamps. This caused an issue when RTTs came back as ver low values of zero. So set delay in config as at least 1ms. 
    {
        // calculate sum of RTTs in this step
        double RTTSum = 0;
        for (double value : RTTMI)
        {
            RTTSum += value;
        }
        initialAverageDelayOfPackets = RTTSum / RTTMI.size(); 
    }
    else
    { 
        // no acks were recieved initially so set reset to be 20ms.TODO: rethink this - this will never happen as the RL agent is only init when CA first starts which can only happen when acks are recieved
        initialAverageDelayOfPackets = 0.005;
    }
    
    return initialAverageDelayOfPackets;
}


}    // namespace learning
#endif