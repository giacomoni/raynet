#ifndef __INET_RdpSinkApp_H
#define __INET_RdpSinkApp_H

#include <inet/common/INETDefs.h>
#include <inet/common/TimeTag_m.h>
#include <inet/common/lifecycle/LifecycleOperation.h>
#include <inet/common/lifecycle/ILifecycle.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "../../transportlayer/contract/rdp/RdpSocket.h"
#include "../../transportlayer/rdp/rdp_common/RdpHeader.h"
namespace inet {

/**
 * Accepts any number of incoming connections, and discards whatever arrives
 * on them.
 */
class INET_API RdpSinkApp : public cSimpleModule
{
protected:
    long bytesRcvd;
    RdpSocket socket;
    //statistics:
    static simsignal_t rcvdPkSignalNDP;
    // MOH: added
    simtime_t tStartAdded;
    simtime_t tEndAdded;

    bool recordStatistics;

    double numRcvTrimmedHeader = 0;
    bool firstDataReceived = true;

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override
    {
        return NUM_INIT_STAGES;
    }
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;
};

} // namespace inet

#endif // ifndef __INET_RdpSinkApp_H

