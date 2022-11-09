#ifndef TRANSPORTLAYER_RDP_FLAVOURS_RLRDP_H_
#define TRANSPORTLAYER_RDP_FLAVOURS_RLRDP_H_
#ifdef RLRDP

#include <inet/common/INETDefs.h>
#include "transportlayer/rdp/RdpAlgorithm.h"
#include "RLInterface.h"
#include "transportlayer/rdp/Rdp.h"
#include "RLRdpAlgStateVariables_m.h"

using namespace learning;


namespace inet {

namespace rdp {

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

    virtual void dataSent(uint32_t fromseq) override;

    virtual void ackSent() override;

    virtual void receivedHeader(unsigned int seqNum) override;

    virtual void receivedData(unsigned int seqNum, bool isMarked) override;

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
#endif