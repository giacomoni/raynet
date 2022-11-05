//
// Copyright (C) 2004 Andras Varga
// Copyright (C) 2010-2011 Zoltan Bojthe
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

#ifndef __INET_RDP_H
#define __INET_RDP_H

#include <map>
#include <set>

#include <inet/common/INETDefs.h>
#include <inet/common/lifecycle/ModuleOperations.h>
#include <inet/common/packet/Packet.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/transportlayer/base/TransportProtocolBase.h>

#include "../contract/rdp/RdpCommand_m.h"
#include "rdp_common/RdpHeader.h"

namespace inet {
namespace rdp {

// Forward declarations:
class RdpConnection;
class RdpSendQueue;
class RdpReceiveQueue;
class INET_API Rdp : public TransportProtocolBase
{
public:
    static simsignal_t numRequestsRTOs;

    enum PortRange
    {
        EPHEMERAL_PORTRANGE_START = 1024, EPHEMERAL_PORTRANGE_END = 5000
    };

    struct SockPair
    {
        L3Address localAddr;
        L3Address remoteAddr;
        int localPort;    // -1: unspec
        int remotePort;    // -1: unspec

        inline bool operator<(const SockPair &b) const
        {
            if (remoteAddr != b.remoteAddr)
                return remoteAddr < b.remoteAddr;
            else if (localAddr != b.localAddr)
                return localAddr < b.localAddr;
            else if (remotePort != b.remotePort)
                return remotePort < b.remotePort;
            else
                return localPort < b.localPort;
        }
    };
    struct SockPairMulticast
    {
        L3Address localAddr;
        int localPort;    // -1: unspec
        int multicastGid;    // multicast group id
        inline bool operator<(const SockPairMulticast &b) const
        {
            if (localAddr != b.localAddr)
                return localAddr < b.localAddr;
            else
                return localPort < b.localPort;
        }
    };
    cMessage *requestTimerMsg = nullptr;

    std::map<int, int> appGateIndexTimeOutMap; // moh: contains num of timeouts for each app
    bool test = true;
    std::map<int, RdpConnection*> requestCONNMap;
    int connIndex = 0;

    long unsigned int counter = 0;
    int timeOut = 0;
    int times = 0;
    bool nap = false;
    bool currentTimerActive = false;

protected:
    typedef std::map<int /*socketId*/, RdpConnection*> RdpAppConnMap;
    typedef std::map<SockPair, RdpConnection*> RdpConnMap;

    RdpAppConnMap rdpAppConnMap;
    RdpConnMap rdpConnMap;
    cOutVector requestTimerStamps;

    ushort lastEphemeralPort = static_cast<ushort>(-1);
    std::multiset<ushort> usedEphemeralPorts;

protected:
    /** Factory method; may be overriden for customizing Tcp */
    virtual RdpConnection* createConnection(int socketId);

    // utility methods
    virtual RdpConnection* findConnForSegment(const Ptr<const RdpHeader> &rdpseg, L3Address srcAddr, L3Address destAddr);
    virtual RdpConnection* findConnForApp(int socketId);
    virtual void segmentArrivalWhileClosed(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address src, L3Address dest);
    virtual void refreshDisplay() const override; //was updateDisplayString()

public:
    bool useDataNotification = false;
    int msl;

public:
    Rdp()
    {
    }
    virtual ~Rdp();

protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override
    {
        return NUM_INIT_STAGES;
    }
    virtual void finish() override;

    virtual void handleSelfMessage(cMessage *message) override;
    virtual void handleUpperCommand(cMessage *message) override;
    virtual void handleUpperPacket(Packet *packet) override;
    virtual void handleLowerPacket(Packet *packet) override;

public:
    /**
     * To be called from RdpConnection when a new connection gets created,
     * during processing of OPEN_ACTIVE or OPEN_PASSIVE.
     */
    virtual void addSockPair(RdpConnection *conn, L3Address localAddr, L3Address remoteAddr, int localPort, int remotePort);

    virtual void removeConnection(RdpConnection *conn); //new
    virtual void sendFromConn(cMessage *msg, const char *gatename, int gateindex = -1); //new

    /**
     * To be called from RdpConnection when socket pair (key for RDPConnMap) changes
     * (e.g. becomes fully qualified).
     */
    virtual void updateSockPair(RdpConnection *conn, L3Address localAddr, L3Address remoteAddr, int localPort, int remotePort);

    /**
     * To be called from RdpConnection: reserves an ephemeral port for the connection.
     */
    virtual ushort getEphemeralPort();

    /**
     * To be called from RdpConnection: create a new send queue.
     */
    virtual RdpSendQueue* createSendQueue();

    virtual RdpReceiveQueue* createReceiveQueue();

    // ILifeCycle:
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    // called at shutdown/crash
    virtual void reset();

    int getMsl()
    {
        return msl;
    }
    virtual bool allConnFinished();
    virtual void updateConnMap();
    virtual void printConnRequestMap();
};

} // namespace rdp
} // namespace inet

#endif // ifndef __INET_RDP_H

