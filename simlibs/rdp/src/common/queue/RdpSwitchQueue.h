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

#ifndef COMMON_QUEUE_RDPSWITCHQUEUE_H_
#define COMMON_QUEUE_RDPSWITCHQUEUE_H_

#include <inet/queueing/base/PacketQueueBase.h>
#include <inet/queueing/contract/IPacketBuffer.h>
#include <inet/queueing/contract/IPacketCollection.h>
#include <inet/queueing/contract/IPassivePacketSink.h>
#include <inet/queueing/contract/IPacketComparatorFunction.h>
#include <inet/queueing/contract/IPacketDropperFunction.h>
#include <inet/queueing/contract/IPassivePacketSource.h>
#include <inet/queueing/contract/IActivePacketSink.h>
#include <inet/queueing/contract/IActivePacketSource.h>
#include <inet/common/INETDefs.h>

namespace inet {
namespace queueing {
/**
 * Drop-front queue. See NED for more info.
 */
class INET_API RdpSwitchQueue : public PacketQueueBase, public cListener
{
  protected:
    // configuration
    int packetCapacity;

    // state
    cPacketQueue dataQueue;
    cPacketQueue headersQueue;
    cPacketQueue synAckQueue;

    cOutVector numTrimmedPacketsVec;

   long dataQueueLength;
   long headersQueueLength;
   long synAckQueueLength;

   unsigned int weight;
   int numTrimmedPkt ;

   cGate *inputGate = nullptr;
   IActivePacketSource *producer = nullptr;

   cGate *outputGate = nullptr;
   IActivePacketSink *collector = nullptr;

   b dataCapacity = b(-1);
    // statistics
    static simsignal_t dataQueueLengthSignal;
    static simsignal_t headersQueueLengthSignal;
    static simsignal_t numTrimmedPktSig;
    static simsignal_t queueingTimeSignal;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;
    virtual bool isOverloaded() const;

  public:
    virtual ~RdpSwitchQueue() {}

    virtual int getMaxNumPackets() const override { return packetCapacity; }
    virtual int getNumPackets() const override;

    virtual b getMaxTotalLength() const override { return dataCapacity; }
    virtual b getTotalLength() const override { return b(dataQueue.getBitLength()); }

    virtual bool isEmpty() const override;
    virtual Packet *getPacket(int index) const override;
    virtual void removePacket(Packet *packet) override;

    virtual bool supportsPushPacket(cGate *gate) const override { return inputGate == gate; }
    virtual bool canPushSomePacket(cGate *gate) const override {return true;};
    virtual bool canPushPacket(Packet *packet, cGate *gate) const override {return true;};
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPopPacket(cGate *gate) const override { return outputGate == gate; }
    virtual bool canPopSomePacket(cGate *gate) const override { return !isEmpty(); }
    virtual Packet *canPopPacket(cGate *gate) const override { return !isEmpty() ? getPacket(0) : nullptr; }
    virtual Packet *popPacket(cGate *gate) override;

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object, cObject *details) override;
    virtual void dropPacket(Packet *packet, PacketDropReason reason, int limit) override;
    virtual void finish() override;
};

}
} // namespace inet

#endif // ifndef __INET_RdpSwitchQueue_H
