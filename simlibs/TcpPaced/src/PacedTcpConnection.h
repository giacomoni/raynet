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

#ifndef SRC_TRANSPORTLAYER_PACEDTCPCONNECTION_H_
#define SRC_TRANSPORTLAYER_PACEDTCPCONNECTION_H_

#include "inet/applications/common/SocketTag_m.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Message.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/networklayer/common/EcnTag_m.h"
#include "inet/networklayer/common/IpProtocolId_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4Tools.h"
#include "inet/transportlayer/contract/tcp/TcpCommand_m.h"
#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/transportlayer/tcp/TcpAlgorithm.h"
#include "inet/transportlayer/tcp/TcpConnection.h"
#include "inet/transportlayer/tcp/TcpReceiveQueue.h"
#include "inet/transportlayer/tcp/TcpSackRexmitQueue.h"
#include "inet/transportlayer/tcp/TcpSendQueue.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/TosTag_m.h"
#include "inet/networklayer/contract/IL3AddressType.h"

#include <queue>
#include <utility>
#include <memory>
#include <tuple>

using namespace inet;
using namespace inet::tcp;
using namespace omnetpp;

class PacedTcpConnection : public TcpConnection
{
public:
    PacedTcpConnection();
    virtual ~PacedTcpConnection();

    std::queue<Packet*> packetQueue;

    cMessage *paceMsg;

    simtime_t intersendingTime;

    /**
     * Process self-messages (timers).
     * Normally returns true. A return value of false means that the
     * connection structure must be deleted by the caller (TCP).
     */
    virtual void initConnection(TcpOpenCommand *openCmd);
    virtual bool processTimer(cMessage *msg);
    virtual void sendToIP(Packet *packet, const Ptr<TcpHeader> &tcpseg);
    virtual void changeIntersendingTime(simtime_t _intersendingTime);

    cOutVector paceValueVec;
    cOutVector bufferedPacketsVec;

private:
    virtual void processPaceTimer();
    void addPacket(Packet *packet);

};

#endif /* SRC_TRANSPORTLAYER_PACEDTCPCONNECTION_H_ */
