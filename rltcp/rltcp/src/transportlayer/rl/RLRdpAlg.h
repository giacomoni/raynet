#ifndef TRANSPORTLAYER_RDP_FLAVOURS_RLRDP_H_
#define TRANSPORTLAYER_RDP_FLAVOURS_RLRDP_H_

#include <inet/common/INETDefs.h>
#include "transportlayer/rdp/RdpAlgorithm.h"
#include "RLInterface.h"
#include "transportlayer/rdp/Rdp.h"

using namespace learning;


namespace inet {

namespace rdp {

class INET_API RLRdpAlgStateVariables : public RdpStateVariables
{
  public:
   // ---- Track if RLInterface has been initialised
    bool rlInitialised;

    // ----- Per Step Variables 
    simtime_t previousStepTimestamp;
    uint32_t numberOfDataPacketsStep;
    uint32_t numberOfTrimmedPacketsStep;
    uint32_t numberOfRecoveredDataPacketsStep;
    uint32_t numberOfTrimmedBytesStep;
    double goodputStep; //Updated everytime we receive a new data packet.

    // ----- Per flow variable
    double goodput;
    double estLinkRate;

    // Sorted STL to keep track of nacked packets. When a packet is nacked, it is inserted in the list.
    // Every time a packet is received, we check if it is a retransmission packet or not.
    // Useful in the case we want to compute RTT only on data that is not retransmitted
    std::set<unsigned int> currentlyNackedPackets;

    //Weights for the reward funtion
    double thrWeight;
    double trimWeight;
    double delayWeight;

    int consecutiveBadSteps;

    double timeCwndChanged;
    int cwndReport;

    //Slow Start
    bool slowStart;



  public:
    RLRdpAlgStateVariables();
};

/**
 * A very-very basic RdpAlgorithm implementation, with hardcoded
 * retransmission timeout and no other sophistication. It can be
 * used to demonstrate what happened if there was no adaptive
 * timeout calculation, delayed acks, silly window avoidance,
 * congestion control, etc.
 */
class INET_API RLRdpAlg : public RdpAlgorithm, public RLInterface
{
  protected:
    RLRdpAlgStateVariables *& state;    // alias to TCLAlgorithm's 'state'

    static simsignal_t cwndSignal;    // will record changes to cwnd
    static simsignal_t ssthreshSignal;    // will record changes to ssthresh

  protected:
    /** Creates and returns a DumbRdpStateVariables object. */
    virtual RdpStateVariables *createStateVariables() override
    {
        return new RLRdpAlgStateVariables();
    }

  public:
    /** Ctor */
    RLRdpAlg();

    virtual ~RLRdpAlg();

    virtual void initialize() override;

    virtual void connectionClosed() override;

    virtual void processTimer(cMessage *timer, RdpEventCode& event) override;

    virtual void dataSent(uint32 fromseq) override;

    virtual void ackSent() override;

    virtual void receivedHeader(unsigned int seqNum) override;

    virtual void receivedData(unsigned int seqNum) override;

    // -------------------- RL Functions -----------------------------
    // Initialise and activate the RL functionality. To be called
    // when Initial Window is fully recveived
    virtual void initRLAgent();
    virtual ObsType computeObservation();
    virtual RewardType computeReward();
    virtual void resetStepVariables();
    virtual void cleanup();
    virtual void decisionMade(ActionType action);
    virtual ObsType getRLState();
    virtual RewardType getReward();
    virtual bool getDone();

};


}
}

#endif