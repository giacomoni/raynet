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

#include <inet/transportlayer/common/L4Tools.h>
#include <inet/common/Protocol.h>
#ifdef WITH_TCP_COMMON
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#endif

#ifdef WITH_UDP
#include <inet/transportlayer/udp/UdpHeader_m.h>
#endif

#ifdef WITH_SCTP
//TODO
#endif

#include "../rdp/rdp_common/RdpHeader_m.h"
#include "../rdp/rdp_common/RdpHeader.h"
namespace inet {

bool isTransportProtocol(const Protocol& protocol)
{
    return
            (protocol == Protocol::tcp)
            || (protocol == Protocol::udp)
            || (protocol == Protocol::rdp)
            // TODO: add other L4 protocols
            ;
}

const Ptr<const TransportHeaderBase> peekTransportProtocolHeader(Packet *packet, const Protocol& protocol, int flags)
{
#ifdef WITH_TCP_COMMON
    if (protocol == Protocol::tcp)
        return packet->peekAtFront<tcp::TcpHeader>(b(-1), flags);
#endif
#ifdef WITH_UDP
    if (protocol == Protocol::udp)
        return packet->peekAtFront<UdpHeader>(b(-1), flags);
#endif
    if (protocol == Protocol::rdp)
        return packet->peekAtFront<rdp::RdpHeader>(b(-1), flags);
    // TODO: add other L4 protocols
    if (flags & Chunk::PF_ALLOW_NULLPTR)
        return nullptr;
    throw cRuntimeError("Unknown protocolTEST6: %s", protocol.getName());
}


const Ptr<TransportHeaderBase> removeTransportProtocolHeader(Packet *packet, const Protocol& protocol)
{
#ifdef WITH_TCP_COMMON
    if (protocol == Protocol::tcp)
        return removeTransportProtocolHeader<tcp::TcpHeader>(packet);
#endif
#ifdef WITH_UDP
    if (protocol == Protocol::udp)
        return removeTransportProtocolHeader<UdpHeader>(packet);
#endif
    if (protocol == Protocol::rdp)
        return removeTransportProtocolHeader<rdp::RdpHeader>(packet);
    // TODO: add other L4 protocols
    throw cRuntimeError("Unknown protocolTEST7: %s", protocol.getName());
}

} // namespace inet
