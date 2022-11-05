#include <inet/common/TimeTag_m.h>
#include "RdpReceiveQueue.h"

namespace inet {

namespace rdp {

Register_Class(RdpReceiveQueue);

RdpReceiveQueue::RdpReceiveQueue()
{
}

RdpReceiveQueue::~RdpReceiveQueue()
{
    receiveBuffer.clear();
}

void RdpReceiveQueue::addPacket(Packet* packet)
{
    receiveBuffer.insert(packet);
}

Packet* RdpReceiveQueue::popPacket()
{
    return check_and_cast<Packet*>(receiveBuffer.pop());
}

std::string RdpReceiveQueue::str() const
{
    std::stringstream out;
    out << "[" << begin << ".." << end << ")" << receiveBuffer;
    return out.str();
}

uint32 RdpReceiveQueue::getBufferStartSeq()
{
    return begin;
}

uint32 RdpReceiveQueue::getBufferEndSeq()
{
    return end;
}

}            // namespace rdp

} // namespace inet

