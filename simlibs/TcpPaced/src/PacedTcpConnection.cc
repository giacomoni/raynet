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
     if (paceMsg)
        delete cancelEvent(paceMsg);

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

void PacedTcpConnection::sendToIP(Packet *tcpSegment, const Ptr<TcpHeader> &tcpHeader)
{ 
    
    // record seq (only if we do send data) and ackno
    if (tcpSegment->getByteLength() > B(tcpHeader->getChunkLength()).get())
        emit(sndNxtSignal, tcpHeader->getSequenceNo());

    emit(sndAckSignal, tcpHeader->getAckNo());

    // final touches on the segment before sending
    tcpHeader->setSrcPort(localPort);
    tcpHeader->setDestPort(remotePort);
    ASSERT(tcpHeader->getHeaderLength() >= TCP_MIN_HEADER_LENGTH);
    ASSERT(tcpHeader->getHeaderLength() <= TCP_MAX_HEADER_LENGTH);
    ASSERT(tcpHeader->getChunkLength() == tcpHeader->getHeaderLength());

    EV_INFO << "Sending: ";
    printSegmentBrief(tcpSegment, tcpHeader);

    // TODO reuse next function for sending

    const IL3AddressType *addressType = remoteAddr.getAddressType();
    tcpSegment->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());

    if (ttl != -1 && tcpSegment->findTag<HopLimitReq>() == nullptr)
        tcpSegment->addTag<HopLimitReq>()->setHopLimit(ttl);

    if (dscp != -1 && tcpSegment->findTag<DscpReq>() == nullptr)
        tcpSegment->addTag<DscpReq>()->setDifferentiatedServicesCodePoint(dscp);

    if (tos != -1 && tcpSegment->findTag<TosReq>() == nullptr)
        tcpSegment->addTag<TosReq>()->setTos(tos);

    auto addresses = tcpSegment->addTagIfAbsent<L3AddressReq>();
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
    tcpSegment->addTagIfAbsent<EcnReq>()->setExplicitCongestionNotification((state->ect && !state->sndAck && !state->rexmit) ? IP_ECN_ECT_1 : IP_ECN_NOT_ECT);

    tcpHeader->setCrc(0);
    tcpHeader->setCrcMode(tcpMain->crcMode);

    insertTransportProtocolHeader(tcpSegment, Protocol::tcp, tcpHeader);

    addPacket(tcpSegment);
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
