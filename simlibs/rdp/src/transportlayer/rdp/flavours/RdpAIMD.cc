
#include "RdpAIMD.h"
#include "../Rdp.h"

namespace inet {
namespace rdp {

Register_Class(RdpAIMD);

simsignal_t RdpAIMD::cwndSignal = cComponent::registerSignal("cwnd");    // will record changes to snd_cwnd
simsignal_t RdpAIMD::ssthreshSignal = cComponent::registerSignal("ssthresh");    // will record changes to ssthresh

RdpAIMDStateVariables::RdpAIMDStateVariables()
{
    internal_request_id = 0;
    request_id = 0;  // source block number (8-bit unsigned integer)

    pacingTime = 1200/1000000;
    numPacketsToGet = 0;
    numPacketsToSend = 0;
    congestionInWindow = false;

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
}

RdpAIMD::RdpAIMD() :
        RdpAlgorithm(), state((RdpAIMDStateVariables*&) RdpAlgorithm::state)
{

}

RdpAIMD::~RdpAIMD()
{

}

void RdpAIMD::initialize()
{

}

void RdpAIMD::connectionClosed()
{

}

void RdpAIMD::processTimer(cMessage *timer, RdpEventCode &event)
{

}

void RdpAIMD::dataSent(uint32 fromseq)
{

}

void RdpAIMD::ackSent()
{

}

void RdpAIMD::receivedHeader(unsigned int seqNum)
{
    EV_INFO << "Header arrived at the receiver" << endl;
    conn->sendNackRdp(seqNum);
    bool noPacketsInFlight = false;
    if(state->outOfWindowPackets > 0){
        state->outOfWindowPackets--;
    }
    else{
        state->sentPullsInWindow--;              //SEND PULLS FOR ALL CWND IF TWO HEADERS IN A ROW (FOR LOOP)
        state->receivedPacketsInWindow++;
    }

    if(state->outOfWindowPackets == 0){
        state->congestionInWindow = true;  //may have to change so certain amount of headers before multiplicative decrease
        state->ssthresh = state->cwnd/2;  //ssthresh, half of cwnd at loss event
        conn->emit(ssthreshSignal, state->ssthresh);
        //state->cwnd = state->cwnd/2;
        state->cwnd = state->cwnd/2;
        if(state->cwnd == 0) state->cwnd = 1;
        //if(state->receivedPacketsInWindow >= state->cwnd){
            state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd;
            state->receivedPacketsInWindow = 0;
            state->sentPullsInWindow = state->cwnd; //0;
            state->waitToStart = true;
        //}
    }

    if (state->numberReceivedPackets == 0 && state->connNotAddedYet == true) { //TODO never going to be called - IW is prioritised REMOVE
        conn->prepareInitialRequest();
        conn->addRequestToPullsQueue();
    }
    if(state->outOfWindowPackets < 0){
        state->congestionInWindow = false;
        for(int i = 0; i < state->outOfWindowPackets * -1; i++){
            conn->addRequestToPullsQueue();

        }
        state->outOfWindowPackets = 0;
    }
    conn->emit(cwndSignal, state->cwnd);
}

void RdpAIMD::receivedData(unsigned int seqNum)
{
    int pullPacketsToSend = 0;
    bool windowIncreased = false;
    //If there are some packets still in the network following a trimmed header arrival
    if(state->outOfWindowPackets > 0){
        state->outOfWindowPackets--;
    }
    else{
        state->receivedPacketsInWindow++;  //TODO are both of these values needed?
        state->sentPullsInWindow--;
    }
    EV_INFO << "Data packet arrived at the receiver - seq num " << seqNum << endl;
    unsigned int arrivedPktSeqNo = seqNum;
    conn->sendAckRdp(arrivedPktSeqNo); //TODO rename method to sendAck
    unsigned int seqNo = seqNum;
    state->numberReceivedPackets++;
    bool onePullNeeded = false;

    if(state->cwnd < state->ssthresh) { //Slow-Start - Exponential Increase
        state->slowStartState = true;
        pullPacketsToSend++;
        state->cwnd++;
    }
    else{
        state->slowStartState = false;
    }

    if(state->sendPulls && state->outOfWindowPackets <= 0){
        if(state->numberReceivedPackets + state->sentPullsInWindow + 1 >= state->numPacketsToGet){
            onePullNeeded = true;
        }
        else if(state->numberReceivedPackets + state->sentPullsInWindow + 2 >= state->numPacketsToGet){ //Only need two pull requests
            pullPacketsToSend++;
            windowIncreased = true;
            onePullNeeded = true;
            state->sendPulls = false;
        }
        else if(((state->receivedPacketsInWindow % state->cwnd) == 0)){ //Congestion Avoidance - Linear Increase
            if(state->slowStartState){
                //pullPacketsToSend = state->cwnd;
                state->receivedPacketsInWindow = 0;
                windowIncreased = true;
                //state->cwnd = state->cwnd + state->cwnd;
                state->congestionInWindow = false;
            }
            else{
                windowIncreased = true;
                state->cwnd = state->cwnd + state->additiveIncreasePackets;
                pullPacketsToSend = pullPacketsToSend + state->additiveIncreasePackets;
                state->receivedPacketsInWindow = 0;
                state->congestionInWindow = false;
            }
        }
    }
    if (state->numberReceivedPackets > state->numPacketsToGet) {
        EV_INFO << "All packets received - finish all connections!" << endl;
        state->connFinished = true;
        conn->getRDPMain()->allConnFinished();
        return;
    }
    else {
        if (state->numberReceivedPackets == 1 && state->connNotAddedYet == true && state->outOfWindowPackets <= 0) {
            //conn->addRequestToPullsQueue(true, false);
            conn->prepareInitialRequest();
            conn->addRequestToPullsQueue();
            for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
                conn->addRequestToPullsQueue();
            }
        }
        else if(state->sendPulls && (state->outOfWindowPackets <= 0) && (state->sentPullsInWindow < state->cwnd) || onePullNeeded){
            if (state->numberReceivedPackets <= (state->numPacketsToGet) && state->connFinished == false) {
                //for loop sending pull for each pullPacketToSend;
                pullPacketsToSend++; //Pull Request to maintain current flow
                for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
                    conn->addRequestToPullsQueue();
                }
            }
        }

        if (state->connFinished == false) {
            EV_INFO << "Sending Data Packet to Application" << endl;
            conn->sendPacketToApp(seqNum);
        }

        state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
        //state->pacingTime = 1200; //seconds
        if(conn->getPullsQueueLength() > 0){
            conn->activatePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
        }
        // All the Packets have been received
        if (state->isfinalReceivedPrintedOut == false) {
            EV_INFO << "Total Received Packets: " << state->numberReceivedPackets << endl;
            EV_INFO << "Total Wanted Packets: " << state->numPacketsToGet << endl;
            if (state->numberReceivedPackets == state->numPacketsToGet || state->connFinished == true) {
                conn->closeConnection();
            }
        }
    }
    conn->emit(cwndSignal, state->cwnd);
}

} // namespace RDP

} // namespace inet

