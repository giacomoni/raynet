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

#include <inet/applications/base/ApplicationPacket_m.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/common/packet/Packet.h>
#include <inet/common/ResultRecorders.h>
#include <inet/common/Simsignals_m.h>
#include <inet/common/TimeTag_m.h>
#include <inet/mobility/contract/IMobility.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/physicallayer/base/packetlevel/FlatReceptionBase.h>
#include <inet/physicallayer/contract/packetlevel/SignalTag_m.h>
#include "ResultFiltersThroughput.h"
namespace inet {
namespace utils {
namespace filters {

Register_ResultFilter("throughputA", ThroughputFilterA);

void ThroughputFilterA::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details)
{
    if (auto packet = dynamic_cast<cPacket*>(object)) {
        const simtime_t now = simTime();
        packets++;
        if (packets >= 1) {
            bytes += packet->getByteLength();
            double throughput = 8 * bytes / (now - lastSignal).dbl();
            fire(this, now, throughput, details);
            lastSignal = now;
            bytes = 0;
            packets = 0;
        }
        else if (now - lastSignal >= interval) { // interval = 0.1
            double throughput = 8 * bytes / interval.dbl();
            fire(this, lastSignal + interval, throughput, details);
            lastSignal = lastSignal + interval;
            bytes = 0;
            packets = 0;
            if (emitIntermediateZeros) {
                while (now - lastSignal >= interval) {
                    lastSignal = lastSignal + interval;
                }
            }
            else {
                if (now - lastSignal >= interval) { // no packets arrived for a long period
                    // zero should have been signaled at the beginning of this packet (approximation)
                    fire(this, now - interval, 0.0, details);
                    lastSignal = now - interval;
                }
            }
            bytes += packet->getByteLength();
        }
        else
            bytes += packet->getByteLength();
    }
}

}

}

}

