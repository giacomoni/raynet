
#ifdef ORCA
#ifndef _RL_ORCA_H_
#define _RL_ORCA_H_

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/tcp/flavours/TcpCubic.h>
#include "OrcaState_m.h"

#include <RLInterface.h>
#include <PacedTcpConnection.h>


#define THR_SCALE_DEEPCC 24
#define THR_UNIT_DEEPCC (1 << THR_SCALE_DEEPCC)

//  reward  = (thr_n_min-5*loss_rate_n_min)/self.max_bw*delay_metric

using namespace inet::tcp;
using namespace inet;
using namespace learning;


class Orca : public TcpCubic, public RLInterface
{
    protected:

    OrcaStateVariables *&state; // alias to TcpAlgorithm's 'state'
        /** Create and return a TcpNewRenoStateVariables object. */

    static simsignal_t avg_thrSignal;
    static simsignal_t thr_cntSignal;
    static simsignal_t max_bwSignal;
    static simsignal_t pacing_rateSignal;
    static simsignal_t lost_bytesSignal;
    static simsignal_t orca_cntSignal;
    static simsignal_t min_rttSignal;
    static simsignal_t avg_urttSignal;

    static simsignal_t feature1Signal;
    static simsignal_t feature2Signal;
    static simsignal_t feature3Signal;
    static simsignal_t feature4Signal;
    static simsignal_t feature5Signal;
    static simsignal_t feature6Signal;
    static simsignal_t feature7Signal;
    static simsignal_t rewardSignal;
    static simsignal_t actionSignal;

    virtual TcpStateVariables* createStateVariables() override
    {
        return new OrcaStateVariables();
    }


public:

    Orca();

    // ---------- Tcp Cubic Functions Override -------------
    virtual void established(bool active) override;
    virtual void receivedDataAck(uint32_t firstSeqAcked) override;
    virtual void receivedDuplicateAck() override;
    virtual void initialize() override;

    // -------------------- RL Functions -----------------------------
    // Initialise and activate the RL functionality. To be called
    // when Initial Window is fully recveived
    virtual void initRLAgent();
    virtual ObsType computeObservation() override;
    virtual RewardType computeReward() override;
    virtual void resetStepVariables() override;
    virtual void cleanup() override;
    virtual void decisionMade(ActionType action) override;
    virtual ObsType getRLState() override;
    virtual RewardType getReward() override;
    virtual bool getDone() override;


};

#endif /* TRANSPORTLAYER_RL_RLTcpCubic_H_ */
#endif