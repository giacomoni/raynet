
#ifndef __INET_DumbRdp_H
#define __INET_DumbRdp_H

#include <inet/common/INETDefs.h>
#include "../RdpAlgorithm.h"

namespace inet {

namespace rdp {

/**
 * State variables for DumbRdp.
 */
class INET_API DumbRdpStateVariables : public RdpStateVariables
{
  public:
    //...
};

/**
 * A very-very basic RdpAlgorithm implementation, with hardcoded
 * retransmission timeout and no other sophistication. It can be
 * used to demonstrate what happened if there was no adaptive
 * timeout calculation, delayed acks, silly window avoidance,
 * congestion control, etc.
 */
class INET_API DumbRdp : public RdpAlgorithm
{
  protected:
    DumbRdpStateVariables *& state;    // alias to TCLAlgorithm's 'state'

  protected:
    /** Creates and returns a DumbRdpStateVariables object. */
    virtual RdpStateVariables *createStateVariables() override
    {
        return new DumbRdpStateVariables();
    }

  public:
    /** Ctor */
    DumbRdp();

    virtual ~DumbRdp();

    virtual void initialize() override;

    virtual void connectionClosed() override;

    virtual void processTimer(cMessage *timer, RdpEventCode& event) override;

    virtual void dataSent(uint32 fromseq) override;

    virtual void ackSent() override;

    virtual void receivedHeader(unsigned int seqNum) override;

    virtual void receivedData(unsigned int seqNum) override;

};

} // namespace RDP

} // namespace inet

#endif // ifndef __INET_DumbRdp_H

