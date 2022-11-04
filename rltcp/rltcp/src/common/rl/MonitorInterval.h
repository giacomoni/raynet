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

#ifndef COMMON_MONITOINTERVAL_H_
#define COMMON_MONITOINTERVAL_H_

#include <memory>
#include <deque>

#include <inet/common/INETDefs.h>

#include "rlUtil.h"

using namespace inet;
using namespace std;

namespace learning {

class MonitorInterval
{
public:
    // MI sequence number
    uint32 miNumber;

    // state->snd_nxt value at the beginning of the MI
    uint32 firstSndMax;

    // state->snd_nxt (or state->snd_max) at the end of MI
    uint32 lastSndMax;

    // whether we already saved the timestamp of the first segment sent in this monitor interval
    bool isFirstSegmentSent;

    // whether the current MI ending tick has been deferred due to lack of data transmitted.
    bool deferredTicking;

    double action; //action taken at the beginning of this monitor interval

    //Time when the last ACK of current MI has been received. Used to check if all data
    //in current MI has been received.
    simtime_t ackForLastSegmentTime;

    //TODO: how can we get rid of the RL specific bit here so that this is reusable? Discuss
    //step associated with this MI
    shared_ptr<Step> step;

    // time the MI started
    simtime_t miStart;

    // time the MI ended
    simtime_t miEnd;

    // time of first segment sent
    simtime_t firstSegmentTime;

    //Avg rtt of the time interval
    double avgRtt;

    MonitorInterval(uint32 _miNumber, uint32 _firstSeqSent, simtime_t _miStart, bool _isFirstPacketSent, bool _deferredTicking, double _action);
    ~MonitorInterval();
};

class MonitorIntervalsHandler
{
public:
    // pointer to the current MI
    shared_ptr<MonitorInterval> currentMi;

    uint32 counter;

    // passed MIs that are waiting for a throughput calculation
    std::deque<shared_ptr<MonitorInterval>> incompleteMis;

    MonitorIntervalsHandler();
    ~MonitorIntervalsHandler();

    void addMI(shared_ptr<MonitorInterval> mi);
};

}

#endif /* COMMON_MONITOINTERVAL_H_ */
