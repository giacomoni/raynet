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

#ifndef TRANSPORTLAYER_TCPRENOPACED_H_
#define TRANSPORTLAYER_TCPRENOPACED_H_

#include <inet/transportlayer/tcp/flavours/TcpReno.h>

#include "PacedTcpConnection.h"

using namespace inet::tcp;

namespace learning {

class PacedTcpReno : public TcpReno
{
protected:
    /** Redefine what should happen on retransmission */
    virtual void processRexmitTimer(TcpEventCode &event) override;

public:
    PacedTcpReno();
    virtual ~PacedTcpReno();

    /** Redefine what should happen when data got acked, to add congestion window management */
    virtual void receivedDataAck(uint32 firstSeqAcked) override;

    /** Redefine what should happen when dupAck was received, to add congestion window management */
    virtual void receivedDuplicateAck() override;
};
}

#endif /* TRANSPORTLAYER_TCPRENOPACED_H_ */
