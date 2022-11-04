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

#include "PacedTcpConnection.h"

Define_Module(PacedTcpConnection);

PacedTcpConnection::PacedTcpConnection()
{

}

PacedTcpConnection::~PacedTcpConnection()
{

    cancelEvent(paceMsg);
    delete paceMsg;
}

bool PacedTcpConnection::processTimer(cMessage *msg)
{
    printConnBrief();
    EV_DETAIL << msg->getName() << " timer expired\n";

    // first do actions
    TcpEventCode event;

    if (msg == paceMsg) {
        processPaceTimer();
    }
    else if (msg == the2MSLTimer) {
        event = TCP_E_TIMEOUT_2MSL;
        process_TIMEOUT_2MSL();
    }
    else if (msg == connEstabTimer) {
        event = TCP_E_TIMEOUT_CONN_ESTAB;
        process_TIMEOUT_CONN_ESTAB();
    }
    else if (msg == finWait2Timer) {
        event = TCP_E_TIMEOUT_FIN_WAIT_2;
        process_TIMEOUT_FIN_WAIT_2();
    }
    else if (msg == synRexmitTimer) {
        event = TCP_E_IGNORE;
        process_TIMEOUT_SYN_REXMIT(event);
    }
    else {
        event = TCP_E_IGNORE;
        tcpAlgorithm->processTimer(msg, event);
    }

    // then state transitions
    return performStateTransition(event);
}

void PacedTcpConnection::initConnection(TcpOpenCommand *openCmd)
{
    TcpConnection::initConnection(openCmd);

    paceMsg = new cMessage("pacing message");

    intersendingTime = 0.005;

    paceValueVec.setName("paceValue");
    bufferedPacketsVec.setName("bufferedPackets");

}

void PacedTcpConnection::addPacket(Packet *packet)
{
    Enter_Method("addPacket");
    if (packetQueue.empty()) {
        if (intersendingTime != 0)
            scheduleAt(simTime() + intersendingTime, paceMsg);
        else
            throw cRuntimeError("Pace is not set.");
    }

    packetQueue.push(packet);
}

void PacedTcpConnection::processPaceTimer()
{
    Enter_Method("ProcessPaceTimer");

    tcpMain->sendFromConn(packetQueue.front(), "ipOut");

    packetQueue.pop();
    bufferedPacketsVec.record(packetQueue.size());

    if (!packetQueue.empty()) {
        if (intersendingTime != 0)
            scheduleAt(simTime() + intersendingTime, paceMsg);
        else
            throw cRuntimeError("Pace is not set.");
    }

}

void PacedTcpConnection::sendToIP(Packet *packet, const Ptr<TcpHeader> &tcpseg)
{
    // record seq (only if we do send data) and ackno
    if (packet->getByteLength() > B(tcpseg->getChunkLength()).get())
        emit(sndNxtSignal, tcpseg->getSequenceNo());

    emit(sndAckSignal, tcpseg->getAckNo());

    // final touches on the segment before sending
    tcpseg->setSrcPort(localPort);
    tcpseg->setDestPort(remotePort);
    ASSERT(tcpseg->getHeaderLength() >= TCP_MIN_HEADER_LENGTH);
    ASSERT(tcpseg->getHeaderLength() <= TCP_MAX_HEADER_LENGTH);
    ASSERT(tcpseg->getChunkLength() == tcpseg->getHeaderLength());
    state->sentBytes = packet->getByteLength();    // resetting sentBytes to 0 if sending a segment without data (e.g. ACK)

    EV_INFO << "Sending: ";
    printSegmentBrief(packet, tcpseg);

    // TBD reuse next function for sending

    IL3AddressType *addressType = remoteAddr.getAddressType();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());

    if (ttl != -1 && packet->findTag<HopLimitReq>() == nullptr)
        packet->addTag<HopLimitReq>()->setHopLimit(ttl);

    if (dscp != -1 && packet->findTag<DscpReq>() == nullptr)
        packet->addTag<DscpReq>()->setDifferentiatedServicesCodePoint(dscp);

    if (tos != -1 && packet->findTag<TosReq>() == nullptr)
        packet->addTag<TosReq>()->setTos(tos);

    auto addresses = packet->addTagIfAbsent<L3AddressReq>();
    addresses->setSrcAddress(localAddr);
    addresses->setDestAddress(remoteAddr);

    // ECN:
    // We decided to use ECT(1) to indicate ECN capable transport.
    //
    // rfc-3168, page 6:
    // Routers treat the ECT(0) and ECT(1) codepoints
    // as equivalent.  Senders are free to use either the ECT(0) or the
    // ECT(1) codepoint to indicate ECT.
    //
    // rfc-3168, page 20:
    // For the current generation of TCP congestion control algorithms, pure
    // acknowledgement packets (e.g., packets that do not contain any
    // accompanying data) MUST be sent with the not-ECT codepoint.
    //
    // rfc-3168, page 20:
    // ECN-capable TCP implementations MUST NOT set either ECT codepoint
    // (ECT(0) or ECT(1)) in the IP header for retransmitted data packets
    packet->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification((state->ect && !state->sndAck && !state->rexmit) ? IP_ECN_ECT_1 : IP_ECN_NOT_ECT);

    tcpseg->setCrc(0);
    tcpseg->setCrcMode(tcpMain->crcMode);

    insertTransportProtocolHeader(packet, Protocol::tcp, tcpseg);

    addPacket(packet);
    bufferedPacketsVec.record(packetQueue.size());
}

void PacedTcpConnection::changeIntersendingTime(simtime_t _intersendingTime)
{
    Enter_Method("changeIntersendingTime");
    ASSERT(_intersendingTime > 0);
    intersendingTime = _intersendingTime;
    EV_TRACE << "New pace: " << intersendingTime << "s" << std::endl;
    paceValueVec.record(intersendingTime);
    if (paceMsg->isScheduled()) {
        simtime_t newArrivalTime = paceMsg->getCreationTime() + intersendingTime;
        if (newArrivalTime < simTime()) {
            cancelEvent(paceMsg);
            scheduleAt(simTime(), paceMsg);
        }
        else {
            cancelEvent(paceMsg);
            scheduleAt(newArrivalTime, paceMsg);
        }
    }

}
