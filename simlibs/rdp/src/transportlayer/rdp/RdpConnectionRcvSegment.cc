#include <string.h>

#include <inet/applications/common/SocketTag_m.h>
#include <inet/common/TimeTag_m.h>
#include "../contract/rdp/RdpCommand_m.h"
#include "../../application/rdpapp/GenericAppMsgRdp_m.h"
#include "../rdp/rdp_common/RdpHeader.h"
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"
#include "RdpReceiveQueue.h"
#include <iostream>
#include <iomanip>

namespace inet {
namespace rdp {

void RdpConnection::sendInitialWindow()
{

    // TODO  we don't do any checking about the received request segment, e.g. check if it's  a request nothing else
    // fetch the next Packet from the encodingPackets list
    EV_TRACE << "RdpConnection::sendInitialWindow";
    std::list<PacketsToSend>::iterator itt;
    if (state->IW > state->numPacketsToSend) {
        state->IW = state->numPacketsToSend;
    }
    for (int i = 1; i <= state->IW; i++) {
        std::tuple<Ptr<RdpHeader>, Packet*> packSeg = sendQueue->getRdpHeader();
        auto rdpseg = std::get<0>(packSeg);
        auto fp = std::get<1>(packSeg);
        if (rdpseg) {
            EV_INFO << "Sending IW packet " << rdpseg->getDataSequenceNumber() << endl;
            rdpseg->setIsDataPacket(true);
            rdpseg->setIsPullPacket(false);
            rdpseg->setIsHeader(false);
            rdpseg->setSynBit(true);
            rdpseg->setAckBit(false);
            rdpseg->setNackBit(false);
            rdpseg->setNumPacketsToSend(state->numPacketsToSend);
            sendToIP(fp, rdpseg);
        }
    }
}

RdpEventCode RdpConnection::process_RCV_SEGMENT(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address src, L3Address dest)
{
    EV_TRACE << "RdpConnection::process_RCV_SEGMENT" << endl;
    //EV_INFO << "Seg arrived: ";
    //printSegmentBrief(packet, rdpseg);
    EV_DETAIL << "TCB: " << state->str() << "\n";
    RdpEventCode event;
    if (fsm.getState() == RDP_S_LISTEN) {
        EV_INFO << "RDP_S_LISTEN processing the segment in listen state" << endl;
        event = processSegmentInListen(packet, rdpseg, src, dest);
        if (event == RDP_E_RCV_SYN) {
            EV_INFO << "RDP_E_RCV_SYN received syn. Changing state to Established" << endl;
            FSM_Goto(fsm, RDP_S_ESTABLISHED);
            EV_INFO << "Processing Segment" << endl;
            event = processSegment1stThru8th(packet, rdpseg);
        }
    }
    else {
        rdpMain->updateSockPair(this, dest, src, rdpseg->getDestPort(), rdpseg->getSrcPort());
        event = processSegment1stThru8th(packet, rdpseg);
    }
    delete packet;
    return event;
}

RdpEventCode RdpConnection::processSegment1stThru8th(Packet *packet, const Ptr<const RdpHeader> &rdpseg)
{
    EV_TRACE << "RdpConnection::processSegment1stThru8th" << endl;
    EV_INFO << "_________________________________________" << endl;
    RdpEventCode event = RDP_E_IGNORE;
    // (S.1)   at the sender: NACK Arrived at the sender, then prepare the trimmed pkt for retranmission
    //        (not to transmit yet just make it to be the first one to transmit upon getting a pull pkt later)
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ NACK Arrived at the sender £££££££££££££££££££ Tx
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    ASSERT(fsm.getState() == RDP_S_ESTABLISHED);
    if (rdpseg->getNackBit() == true) {
        EV_INFO << "Nack arrived at the sender - move data packet to front" << endl;
        sendQueue->nackArrived(rdpseg->getNackNo());
    }

    // (S.2)  at the sender:  ACK arrived, so free the acknowledged pkt from the buffer.
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ ACK Arrived at the sender £££££££££££££££££££ Tx
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    if (rdpseg->getAckBit() == true) {
        EV_INFO << "Ack arrived at the sender - free ack buffer" << endl;
        sendQueue->ackArrived(rdpseg->getAckNo());
    }

    // (S.3)  at the sender: PULL pkt arrived, this pkt triggers either retransmission of trimmed pkt or sending a new data pkt.
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ REQUEST Arrived at the sender £££££££££££££££
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    if (rdpseg->isPullPacket() == true || ((rdpseg->getNackBit() == true) && (state->delayedNackNo > 0))) {
        int requestsGap = rdpseg->getPullSequenceNumber() - state->internal_request_id;
        unsigned int pullRequestNumber = rdpseg->getPullSequenceNumber();
        EV_INFO << "Pull packet arrived at the sender - request gap " << requestsGap << endl;
        if(state->delayedNackNo > 0){
            requestsGap = 1;
            --state->delayedNackNo;
        }
        if (requestsGap >= 1) {
            //  we send Packets  based on requestsGap value
            // if the requestsGap is smaller than 1 that means we received a delayed request which we need to  ignore
            // as we have assumed it was lost and we send extra Packets previously
            for (int i = 1; i <= requestsGap; i++) {
                ++state->internal_request_id;
                std::tuple<Ptr<RdpHeader>, Packet*> headPack = sendQueue->getRdpHeader();
                const auto &rdpseg = std::get<0>(headPack);
                Packet *fp = std::get<1>(headPack);
                if (rdpseg) {
                    EV_INFO << "Sending data packet - " << rdpseg->getDataSequenceNumber() << endl;
                    rdpseg->setIsDataPacket(true);
                    rdpseg->setIsPullPacket(false);
                    rdpseg->setIsHeader(false);
                    rdpseg->setAckBit(false);
                    rdpseg->setNackBit(false);
                    rdpseg->setSynBit(false);
                    rdpseg->setNumPacketsToSend(state->numPacketsToSend);
                    rdpseg->setPullSequenceNumber(pullRequestNumber); //Echo the PR number
                    sendToIP(fp, rdpseg);
                }
                else {
                    EV_WARN << "No Rdp header within the send queue!" << endl;
                    ++state->delayedNackNo;
                    --state->internal_request_id;
                    //EV_INFO << "No Rdp header within the send queue!" << endl;
                    //--state->internal_request_id;
                    //state->internal_request_id = state->internal_request_id - requestsGap;
                    //--state->request_id;

                }
            }
        }
        else if (requestsGap < 1) {
            EV_INFO << "Delayed pull arrived --> ignore it" << endl;
        }
    }
    // (R.1)  at the receiver
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$  HEADER arrived   $$$$$$$$$$$$$$$$   Rx
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // header arrived at the receiver==> send new request with pacing (fixed pacing: MTU/1Gbps)
    if (rdpseg->isHeader() == true && rdpseg->isDataPacket() == false) { // 1 read, 2 write
        //state->receivedPacketsInWindow++;
        computeRtt(rdpseg->getPullSequenceNumber(), true);
        state->numRcvTrimmedHeader++;
        emit(trimmedHeadersSignal, state->numRcvTrimmedHeader);
        rdpAlgorithm->receivedHeader(rdpseg->getDataSequenceNumber());
        
    }
    // (R.2) at the receiver
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$  data pkt arrived at the receiver  $$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    if (rdpseg->isDataPacket() == true && rdpseg->isHeader() == false) {
        computeRtt(rdpseg->getPullSequenceNumber(), false);
        simtime_t interarrivaltime;
        if(state->lastDataPacketArrived == 0)
            interarrivaltime = state->minRtt;
        else
            interarrivaltime = simTime() - state->lastDataPacketArrived;
        if(interarrivaltime.dbl() > 0)
            state->bandwidthEstimator.addSample((b(packet->getTotalLength()).get())/interarrivaltime.dbl(), simTime());
        if(b(packet->getTotalLength()).get() != 1500*8)
            std::cout << B(packet->getTotalLength()).get() << std::endl;
        if(interarrivaltime.dbl() < b(packet->getTotalLength()).get()/80000000)
            std::cout << interarrivaltime.dbl() << std::endl;
        state->lastDataPacketArrived = simTime();
        Packet* packClone = packet->dup();
        receiveQueue->addPacket(packClone);
        rdpAlgorithm->receivedData(rdpseg->getDataSequenceNumber());
        
    }

    return event;
}

void RdpConnection::addRequestToPullsQueue() //TODO remove pacePacket bool
{
    EV_TRACE << "RdpConnection::addRequestToPullsQueue" << endl;
    ++state->request_id;
    char msgname[16];
    sprintf(msgname, "PULL-%d", state->request_id);
    Packet *rdppack = new Packet(msgname);

    const auto &rdpseg = makeShared<RdpHeader>();
    rdpseg->setIsDataPacket(false);
    rdpseg->setIsPullPacket(true);
    rdpseg->setIsHeader(false);
    rdpseg->setSynBit(false);
    rdpseg->setAckBit(false);
    rdpseg->setNackBit(false);
    rdpseg->setPullSequenceNumber(state->request_id);
    rdppack->insertAtFront(rdpseg);
    pullQueue.insert(rdppack);

    EV_INFO << "Adding new request to the pull queue -- pullsQueue length now = " << pullQueue.getLength() << endl;
    EV_INFO << "Requesting Pull Timer" << endl;
}
void RdpConnection::sendRequestFromPullsQueue()
{
    EV_TRACE << "RdpConnection::sendRequestFromPullsQueue" << endl;
    if (pullQueue.getByteLength() > 0) {
        state->sentPullsInWindow++;
        Packet *fp = check_and_cast<Packet*>(pullQueue.pop());
        auto rdpseg = fp->removeAtFront<rdp::RdpHeader>();
        state->pullRequestsTransmissionTimes.insert(std::pair<unsigned int, simtime_t>(rdpseg->getPullSequenceNumber(), simTime()));     
        state->lastPullTime = simTime();
        sendToIP(fp, rdpseg);
    }
}

int RdpConnection::getPullsQueueLength()
{
    int len = pullQueue.getLength();
    return len;
}

bool RdpConnection::isConnFinished()
{
    return state->connFinished;
}

int RdpConnection::getNumRcvdPackets()
{
    return state->numberReceivedPackets;
}

void RdpConnection::setConnFinished()
{
    state->connFinished = true;
}

void RdpConnection::sendPacketToApp(unsigned int seqNum){
    Packet* packet = receiveQueue->popPacket();
    Ptr<Chunk> msgRx;
    auto data = packet->peekData();
    auto regions = data->getAllTags<CreationTimeTag>();
    simtime_t creationTime;
    for (auto& region : regions) {
        creationTime = region.getTag()->getCreationTime(); // original time
        break; //should only be one timeTag
    }
    msgRx = packet->removeAll();

    std::list<PacketsToSend>::iterator itR;  // received iterator
    itR = receivedPacketsList.begin();
    std::advance(itR, seqNum); // increment the iterator by esi
    // MOH: Send any received Packet to the app, just for now to test the Incast example, this shouldn't be the normal case
    std::string packetName = "DATAPKT-" + std::to_string(seqNum);
    Packet *newPacket = new Packet(packetName.c_str(), msgRx);
    newPacket->addTag<CreationTimeTag>()->setCreationTime(creationTime);
    PacketsToSend receivedPkts;
    receivedPkts.pktId = seqNum;
    receivedPkts.msg = newPacket;
    receivedPacketsList.push_back(receivedPkts);
    newPacket->setKind(RDP_I_DATA); // TBD currently we never send RDP_I_URGENT_DATA
    newPacket->addTag<SocketInd>()->setSocketId(socketId);
    EV_INFO << "Sending to App packet: " << newPacket->str() << endl;
    sendToApp(newPacket);
    delete packet;
}

void RdpConnection::prepareInitialRequest(){
    getRDPMain()->requestCONNMap[getRDPMain()->connIndex] = this; // moh added
    getRDPMain()->connIndex++;
    state->connNotAddedYet = false;
    getRDPMain()->nap = true;    //TODO change to setter method
}

void RdpConnection::closeConnection(){
    std::list<PacketsToSend>::iterator iter; // received iterator
    iter = receivedPacketsList.begin();
    while (iter != receivedPacketsList.end()) {
        iter++;
    }
    cancelRequestTimer();
    EV_INFO << " numRcvTrimmedHeader:    " << state->numRcvTrimmedHeader << endl;
    EV_INFO << "CONNECTION FINISHED!" << endl;
    sendIndicationToApp(RDP_I_PEER_CLOSED); // this is ok if the sinkApp is used by one conn
    state->isfinalReceivedPrintedOut = true;
    
}
RdpEventCode RdpConnection::processSegmentInListen(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address srcAddr, L3Address destAddr)
{
    EV_DETAIL << "Processing segment in LISTEN" << endl;

    if (rdpseg->getSynBit()) {
        EV_DETAIL << "SYN bit set: filling in foreign socket" << endl;
        rdpMain->updateSockPair(this, destAddr, srcAddr, rdpseg->getDestPort(), rdpseg->getSrcPort());
        // this is a receiver
        state->numPacketsToGet = rdpseg->getNumPacketsToSend();
        return RDP_E_RCV_SYN; // this will take us to SYN_RCVD
    }
    EV_WARN << "Unexpected segment: dropping it" << endl;
    return RDP_E_IGNORE;
}

void RdpConnection::segmentArrivalWhileClosed(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address srcAddr, L3Address destAddr)
{
    EV_TRACE << "RdpConnection::segmentArrivalWhileClosed" << endl;
    EV_INFO << "Seg arrived: " << endl;
    printSegmentBrief(packet, rdpseg);
    // This segment doesn't belong to any connection, so this object
    // must be a temp object created solely for the purpose of calling us
    ASSERT(state == nullptr);
    EV_INFO << "Segment doesn't belong to any existing connection" << endl;
    EV_FATAL << "RdpConnection::segmentArrivalWhileClosed should not be called!";
}

}    // namespace rdp

} // namespace inet

