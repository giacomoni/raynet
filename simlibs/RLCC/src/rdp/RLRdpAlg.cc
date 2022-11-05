
#include "RLRdpAlg.h"
#include <cmath>


namespace inet {
namespace rdp {

Register_Class(RLRdpAlg);

simsignal_t RLRdpAlg::cwndSignal = cComponent::registerSignal("cwnd");    // will record changes to snd_cwnd
simsignal_t RLRdpAlg::ssthreshSignal = cComponent::registerSignal("ssthresh");    // will record changes to ssthresh


RLRdpAlgStateVariables::RLRdpAlgStateVariables()
{
    
    //Number of packets currently in the network. Initial value is set to the Initial Window. For each trimmed header or data paket received,
    //this vlue is dereased. New PULL REQUESTS to send will equal  cwnd - packetsInFlight
    packetsInFlight = 0;
    
    rlInitialised = false;

  
    previousStepTimestamp = SIMTIME_ZERO;
    numberOfDataPacketsStep = 0;
    numberOfTrimmedPacketsStep = 0;
    numberOfRecoveredDataPacketsStep = 0;
    numberOfTrimmedBytesStep = 0;
    goodputStep = 0; //Updated everytime we receive a new data packet.
    estLinkRate = 0;

    // ----- Per flow variable
    goodput = 0;

    consecutiveBadSteps = 0;

    timeCwndChanged = 0;
    cwndReport = 0;
    slowStart = true;

    bandwidthEstimator.setWindowSize(10);

    pacingTime = 0;
}

RLRdpAlg::RLRdpAlg() :
        RdpAlgorithm(), state((RLRdpAlgStateVariables*&) RdpAlgorithm::state)
{

}

RLRdpAlg::~RLRdpAlg()
{

}

void RLRdpAlg::initialize()
{

}

void RLRdpAlg::connectionClosed()
{

}

void RLRdpAlg::processTimer(cMessage *timer, RdpEventCode &event)
{

}

void RLRdpAlg::dataSent(uint32 fromseq)
{

}

void RLRdpAlg::ackSent()
{

}

void RLRdpAlg::initRLAgent(){
    this->setOwner(this->conn);
    RLInterface::initialise();

    // Generate ID for this agent
    std::string s(this->conn->getFullPath());
    std::string token = s.substr(s.find("-")+1);

    this->setStringId(token);

    // ----- Per Step Variables 
    state->previousStepTimestamp = simTime();
    state->numberOfTrimmedPacketsStep = 0;
    state->numberOfTrimmedBytesStep = 0;
    state->goodputStep = 0; //Updated everytime we receive a new data packet.
    state->numberOfDataPacketsStep = 0;
    state->numberOfRecoveredDataPacketsStep = 0;

    // ----- Per flow variable
    state->goodput = 0;
    state->rlInitialised = true;
    state->timeCwndChanged = simTime().dbl();
    state->cwndReport = state->cwnd;

    state->thrWeight = conn->getRDPMain()->par("thrWeight");
    state->trimWeight = conn->getRDPMain()->par("trimWeight");
    state->delayWeight = conn->getRDPMain()->par("delayWeight");
}

void RLRdpAlg::resetStepVariables(){
    state->previousStepTimestamp = simTime();
    state->numberOfDataPacketsStep = 0;
    state->numberOfTrimmedPacketsStep = 0;
    state->numberOfTrimmedBytesStep = 0;
    state->numberOfRecoveredDataPacketsStep = 0;
    state->sRttStep = SIMTIME_ZERO;
    state->minRttStep = SIMTIME_ZERO;
    state->goodputStep = 0; //Updated everytime we receive a new data packet.
    state->rttvarStep = SIMTIME_ZERO;
}


void RLRdpAlg::receivedHeader(unsigned int seqNum)
{   
    
    conn->sendNackRdp(seqNum);
    state->numRcvTrimmedHeader++;


    // Store seqeNum of lost packet
    state->currentlyNackedPackets.insert(seqNum);

    //TODO: Distinguish slow start case and congestion avoidance

    // Update per step variables
    state->numberOfTrimmedPacketsStep = state->numberOfTrimmedPacketsStep + 1;

   if(state->numberReceivedPackets > 0 && state->connNotAddedYet == false){
         state->packetsInFlight =  state->packetsInFlight - 1;

    }
    else{
       EV_TRACE  << "+++++++++++++++++++++ void RLRdpAlg::receivedHeader(): THIS CASE SHOULD NOT HAPPEN ++++++++++++++++++++++++++" << std::endl;
    }

    //If there is space in the window, add packets
         if (state->cwnd > state->packetsInFlight){
            for (int i = 0; i < (state->cwnd - state->packetsInFlight); i++) {
                conn->addRequestToPullsQueue();
                state->packetsInFlight = state->packetsInFlight + 1;
                }
            }

     if(state->slowStart){
        
        if(!state->rlInitialised && (state->sRtt.dbl() != 0)){
            initRLAgent();
            // Send the initial step size along
            cObject* simtime = new cSimTime(2*state->rttPropEstimator.getMin());
            conn->emit(this->registerSig, stringId.c_str(), simtime);
            state->slowStart = false;
            state->cwnd = round(state->cwnd/2);
    }

     }

     if(conn->getPullsQueueLength() > 0){
            conn->activatePullTimer();
        }

 
}

void RLRdpAlg::receivedData(unsigned int seqNum)
{
    // ----- On RECEIVER: received data packet
    conn->sendAckRdp(seqNum);
    state->numberReceivedPackets++;

    // ----- On RECEIVER: all data received
    if (state->numberReceivedPackets > state->numPacketsToGet) {
        state->connFinished = true;
        return;
    }else{

        // ----- On RECEIVER: first packet received and it's data
        if (state->numberReceivedPackets == 1 && (state->connNotAddedYet == true)) {
            // Initialization of packets in flight: Initial window minus the one just received
            state->packetsInFlight = state->IW - 1;
            conn->prepareInitialRequest();
        }
        else if(state->numberReceivedPackets > 1 && (state->connNotAddedYet == false)){
            if(state->rttPropEstimator.getMin() > 0)
                state->bandwidthEstimator.setWindowSize(10);
            state->packetsInFlight =  state->packetsInFlight - 1;
            if (state->minRtt > 0){
                state->estLinkRate = state->bandwidthEstimator.getMax();
            }
            
                // Check if it a retransmitted data packet
            auto search = state->currentlyNackedPackets.find(seqNum);
            if (search != state->currentlyNackedPackets.end()) {
                //--- Retransmission packet. Now it has been received, remove from set.
                state->numberOfRecoveredDataPacketsStep = state->numberOfRecoveredDataPacketsStep + 1;
                state->currentlyNackedPackets.erase(seqNum);
            } else {
                if(state->rlInitialised){
                    state->goodputStep = 8*(double(state->numberOfDataPacketsStep)*1500/((simTime() - state->previousStepTimestamp).dbl()));
                }
                else{
                    state->goodputStep = 0;
                    state->previousStepTimestamp = simTime();
                }

                state->numberOfDataPacketsStep = state->numberOfDataPacketsStep + 1;
            }
            
        }
        else{
            EV_TRACE<< "+++++++++++++++++++++ void RLRdpAlg::receivedData(unsigned int seqNum): THIS CASE SHOULD NOT HAPPEN ++++++++++++++++++++++++++" << std::endl;
        }


        //TODO: what about reordering?
        if (state->connFinished == false) {
            conn->sendPacketToApp(seqNum);
        }

        if(state->slowStart){
            state->cwnd = state->cwnd+1;

        }

        double alpha=0.0;
        double new_pacing = state->rttPropEstimator.getMin()/state->cwnd;

        // Notify change in pacing and let the conn reschedule the current pacing timer. 
        // conn->paceChanged(new_pacing);

        state->pacingTime = alpha*state->pacingTime + (1-alpha)*new_pacing;


        //If there is space in the window, add packets
            if (state->cwnd > state->packetsInFlight){
            for (int i = 0; i < (state->cwnd - state->packetsInFlight); i++) {
                conn->addRequestToPullsQueue();
                state->packetsInFlight = state->packetsInFlight + 1;
                }
            }
        
        if(conn->getPullsQueueLength() > 0){
            conn->activatePullTimer();
        }

        // All the Packets have been received
        if (state->isfinalReceivedPrintedOut == false) {

            if (state->numberReceivedPackets == state->numPacketsToGet || state->connFinished == true) {
                conn->closeConnection();
                this->done = true;
                conn->emit(this->unregisterSig, stringId.c_str());
            
            }
        }
    }
    conn->emit(cwndSignal, state->cwnd);
}


ObsType RLRdpAlg::computeObservation(){

    // // Received at least one trimmed header
    // double dataHeaderRttRatio;
    // if(state->numRcvTrimmedHeader > 0){
    //     dataHeaderRttRatio = state->sRtt.dbl()/state->minRttHeader.dbl() - 1;
    // }
    // else{
    //     dataHeaderRttRatio = 0.0;
    // }

    double trimPortion;
    // No data nor trimmed packets received?
    if ((state->numberOfTrimmedPacketsStep+state->numberOfDataPacketsStep) <= 0){
        trimPortion = 1;
    }
    else{
        trimPortion = double(state->numberOfTrimmedPacketsStep)/(double(state->numberOfTrimmedPacketsStep)+double(state->numberOfDataPacketsStep));

    }

     if(trimPortion >=0.85){
         state->consecutiveBadSteps++;
     }else{
         state->consecutiveBadSteps = 0;
     }

    //Early Termination Trick for heavily congested networks.
    if(state->consecutiveBadSteps >= 3){
        conn->closeConnection();
        this->done = true;
        conn->emit(this->unregisterSig, stringId.c_str());
       
    }
    
    float rttNorm;
    if ((state->rttPropEstimator.getMax() - state->rttPropEstimator.getMin()) != 0)
        rttNorm = (state->sRttStep.dbl() - state->rttPropEstimator.getMin())/(state->rttPropEstimator.getMax() - state->rttPropEstimator.getMin());
    else
        rttNorm = 0;
    
    float currentBdp = state->bandwidthEstimator.getMax()*state->rttPropEstimator.getMin()/(8*1500);


    return {float(double(state->goodputStep)/double(state->bandwidthEstimator.getMax())),
            trimPortion,
            float(rttNorm),
            float(state->cwnd)/1000000,
            
            state->timeCwndChanged,
            float(state->cwndReport),
            float(state->pacingTime),
            float(state->bandwidthEstimator.getMean()),
            float(state->bandwidthEstimator.getStd()),
            float(state->rttPropEstimator.getMean()),
            float(state->rttPropEstimator.getStd()),
            float(state->rttPropEstimator.getMin()),
            float(state->bandwidthEstimator.getMax()),
            float(double(state->numRcvTrimmedHeader)/(double(state->numRcvTrimmedHeader) + double(state->numberReceivedPackets))),
            float(double(state->numRcvTrimmedHeader)/(double(state->numberReceivedPackets))),
            float(state->sRttStep.dbl()),
            float(state->goodputStep)};
}

RewardType RLRdpAlg::computeReward(){
    double trimPortion;
    if ((state->numberOfTrimmedPacketsStep+state->numberOfDataPacketsStep) <= 0){
        trimPortion = 1;
    }
    else{
        trimPortion = double(state->numberOfTrimmedPacketsStep)/(double(state->numberOfTrimmedPacketsStep)+double(state->numberOfDataPacketsStep));

    }
    
    //Maximum reward achieved is 1 when throughput is 1Gbps and delay = minDelay.
    //When no data is received, we set the reward to
    RewardType reward;
    double throughputComponent, delaycomponent;

    if(trimPortion < 1){
        if(state->bandwidthEstimator.getMax() > 0){
            throughputComponent = (state->thrWeight*(state->goodputStep/state->bandwidthEstimator.getMax()) - state->trimWeight*trimPortion);
            if(throughputComponent >= 0.96){
                if(( state->rttPropEstimator.getMax() - state->rttPropEstimator.getMin()) != 0){
                    double delayComponent = 2*(state->rttPropEstimator.getMin()/state->sRttStep.dbl())*(1.0 - ((state->sRttStep.dbl() - state->rttPropEstimator.getMin())/(state->rttPropEstimator.getMax() - state->rttPropEstimator.getMin())));
                    reward = throughputComponent*delayComponent;
                }else{
                    reward = throughputComponent; 
                }
                // reward = (state->thrWeight*state->goodputStep - state->trimWeight*trimPortion);
            }else{
                reward = throughputComponent;
            }
        }else{
                reward = 0;
        }
        
    }else{
        reward = -1;
    }
    //Penalize agent when early termination occurs due to heavy congestion 
    if(state->consecutiveBadSteps >= 3){
        reward = -1;
        state->consecutiveBadSteps = 0;
    }
    return reward;
}

void RLRdpAlg::cleanup(){
}

void RLRdpAlg::decisionMade(ActionType action){

    //Ignore if done
    if(!done){
    state->cwnd = max(int(floor(pow(2, action) * state->cwnd)), 1);


    state->timeCwndChanged = simTime().dbl();
    state->cwndReport = state->cwnd;

    double alpha=0.0;
    double new_pacing = state->rttPropEstimator.getMin()/state->cwnd;
    state->pacingTime = alpha*state->pacingTime + (1-alpha)*new_pacing;

    simtime_t stepSize = state->rttPropEstimator.getMin();
    // Reschedule next step according to new sRTT
     if (stepSize > SIMTIME_ZERO){
        cString* c_name = new cString(stringId);
        conn->emit(modifyStepSizeSig, 2*stepSize.dbl(), c_name);
    }

        lastMiAction = action;
    }

}

ObsType RLRdpAlg::getRLState(){

}

RewardType RLRdpAlg::getReward(){

}

bool RLRdpAlg::getDone(){


}

} // namespace RDP

} // namespace inet

