//
// Copyright (C) 2004 Andras Varga
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

#ifndef __INET_RdpSendQueue_H
#define __INET_RdpSendQueue_H

#include <inet/common/INETDefs.h>
#include <inet/common/packet/ChunkQueue.h>
#include <inet/common/packet/Packet.h>

#include "../../application/rdpapp/GenericAppMsgRdp_m.h"
#include "../rdp/RdpConnection.h"
#include "../rdp/rdp_common/RdpHeader_m.h"
#include "../rdp/rdp_common/RdpHeader.h"

namespace inet {

namespace rdp {

class INET_API RdpSendQueue : public cObject
{
protected:
    RdpConnection *conn = nullptr;    // the connection that owns this queue
    uint32 begin = 0;    // 1st sequence number stored
    uint32 end = 0;    // last sequence number stored +1

    cPacketQueue dataToSendQueue;      // dataBuffer
    //cPacketQueue sentDataQueue;
    std::map<int, cPacket*> sentDataQueue;

public:
    /**
     * Ctor.
     */
    RdpSendQueue();

    /**
     * Virtual dtor.
     */
    virtual ~RdpSendQueue();

    virtual cPacketQueue& getDataToSendQueue()
    {
        return dataToSendQueue;
    }
    /**
     * Set the connection that owns this queue.
     */
    virtual void setConnection(RdpConnection *_conn)
    {
        conn = _conn;
    }

    /**
     * Initialize the object. The dataToSendQueue will be filled with data packets given the numPacketsToSend
     * value. This should only be called once for each flow.
     */
    virtual void init(int numPacketsToSend, B mss);

    /**
     * Returns a string with the region stored.
     */
    virtual std::string str() const override;

    /**
     * Returns the sequence number of the first byte stored in the buffer.
     */
    virtual uint32 getBufferStartSeq();

    /**
     * Returns the sequence number of the last byte stored in the buffer plus one.
     * (The first byte of the next send operation would get this sequence number.)
     */
    virtual uint32 getBufferEndSeq();

    virtual const std::tuple<Ptr<RdpHeader>, Packet*> getRdpHeader();

    /**
     * Called when an ACK has arrived at the sender. Frees the sentDataQueue buffer.
     */
    virtual void ackArrived(unsigned int ackNum);

    /**
     * Called when a NACK has arrived at the sender. Pushes the NACKed packet to the front of the dataToSendQueue.
     */
    virtual void nackArrived(unsigned int nackNum);

    /**
     * Utility function: returns how many bytes are available in the queue, from
     * (and including) the given sequence number.
     */
    inline ulong getBytesAvailable(uint32 fromSeq)
    {
        uint32 bufEndSeq = getBufferEndSeq();
        return seqLess(fromSeq, bufEndSeq) ? bufEndSeq - fromSeq : 0;
    }

//    void removeFromDataQueueToSentQueue(Chunk::Iterator iter);
    void moveFrontDataQueue(unsigned int sequenceNumber);
};

} // namespace RDP

} // namespace inet

#endif // ifndef __INET_RdpSendQueue_H

