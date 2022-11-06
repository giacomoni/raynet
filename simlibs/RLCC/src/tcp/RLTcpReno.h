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

#ifndef TRANSPORT_TCPRLRENO_H_
#define TRANSPORT_TCPRLRENO_H_

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/tcp/Tcp.h>
#include <inet/transportlayer/tcp/TcpSendQueue.h>
#include <inet/transportlayer/tcp/flavours/TcpReno.h>

#include "MonitorInterval.h"
#include "PacedTcpConnection.h"

#include "RLInterface.h"

using namespace inet::tcp;
using namespace inet;

namespace learning {
/**
 * Implements RLReno.
 */
class RLTcpReno : public TcpReno, public RLInterface
{
protected:

    // is the underlying TcpConnection paced or not
    bool isConnectionPaced;

    // am I running on active open (client) or passive open connection (server)
    bool isActive;

    // number of duplicate acknowledgements received in a single time interval
    uint32 dupAcks;

    // maximum allowed congestion window (user parameter for RL)
    uint32 maxLearnWindow;

    // state variables as defined in Tcp Ex Machina
    std::shared_ptr<Ewma> interArrivalEwmaPtr, interSendEwmaPtr, rttEwmaPtr;

    // Should store the reward of the previous MI. However, given that there can be many MIs waiting for an ACk, this var stores the last reqrd calculated
    double lastMIReward;

    // most recent RTT measurement
    double lastMIavgRTT;

    // RTT values in a single MI used to compute reegression.
    std::vector<double> rttsMi;

    // When the RTT valuues have  been computed.
    std::vector<double> rttsMiTimes;

    // OMNeT++ message used to schedule monitor intervals
    cMessage *monitorInterval;

    // utility object that keeps track of Monitor Intervals and relevant calculations
    MonitorIntervalsHandler miHandler;

    //Signals for result recording
    simsignal_t throughputSignal;
    simsignal_t actionSignal;
    simsignal_t dupAcksSignal;
    simsignal_t rttGradientSignal;
    simsignal_t tickSignal;
    simsignal_t miQueueSizeSignal;

public:
    RLTcpReno();
    ~RLTcpReno();

    // overridden methods from TcpReno
    virtual void initialize() override;

    virtual void established(bool active) override;

    virtual bool sendData(bool sendCommandInvoked) override;

    virtual void receivedDataAck(uint32 firstSeqAcked) override;
    virtual void receivedDuplicateAck() override;

    virtual void processTimer(cMessage *timer, TcpEventCode &event) override;
    virtual void processRexmitTimer(TcpEventCode &event) override;

    //We override this function because we want to store every single RTT computed.
    virtual void rttMeasurementComplete(simtime_t tSent, simtime_t tAcked) override;

    // Handles the MI timer. Mainly call the tick() function.
    virtual void processMonitorIntervalTimer(TcpEventCode &event);

    // Implements functionality related to starting/ending a Monitor Interval.
    void tick();

    // Takes in an empty vector, and fills it with the current observation of the state.
    void computeObservation(std::vector<double> &obs);

    // Compute the current RL state and request an action to the agent given the current state.
    virtual void act();

    // Call back from RLInterface. Called when the action from the agent has been received.
    virtual void decisionMade(ActionType action) override;

    virtual void cleanup() override;

    virtual ObsType getRLState() override;

    virtual RewardType getReward() override;

    virtual bool getDone() override;
    virtual void resetStepVariables()override{} ;
    virtual ObsType computeObservation()override{};
      virtual RewardType computeReward()override{};


};

} // namespace learning

#endif /* TRANSPORT_TCPRLRENO_H_ */
