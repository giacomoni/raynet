//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef COMMON_RESULTFILTERSTHROUGHPUT_H_
#define COMMON_RESULTFILTERSTHROUGHPUT_H_

#include <inet/common/INETDefs.h>

namespace inet {

namespace utils {

namespace filters {

/**
 * Filter that expects a cPacket and outputs the throughput as double.
 * Throughput is computed for the *past* interval every 0.1s or 100 packets,
 * whichever comes first.
 *
 * Note that this filter is unsuitable for interactive use (with instrument figures,
 * for example), because zeroes for long silent periods are only emitted retroactively,
 * when the silent period (or the simulation) is over.
 *
 * Recommended interpolation mode: backward sample-hold.
 */
class INET_API ThroughputFilterA : public cObjectResultFilter
{
protected:
    simtime_t interval = 0.1;
    int packetLimit = 100;
    bool emitIntermediateZeros = true;

    simtime_t lastSignal = 0;
    double bytes = 0;
    int packets = 0;

protected:
    void emitThroughput(simtime_t endInterval, cObject *details);
public:
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
};

}

}

}

#endif /* COMMON_RESULTFILTERSTHROUGHPUT_H_ */
