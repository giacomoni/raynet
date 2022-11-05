#include <string.h>
#include <algorithm>
#include <inet/networklayer/contract/IL3AddressType.h>
#include <inet/networklayer/common/IpProtocolId_m.h>
#include <inet/applications/common/SocketTag_m.h>
#include <inet/common/INETUtils.h>
#include <inet/common/packet/Message.h>
#include <inet/networklayer/common/EcnTag_m.h>
#include <inet/networklayer/common/IpProtocolId_m.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/networklayer/common/HopLimitTag_m.h>
#include <inet/common/Protocol.h>
#include <inet/common/TimeTag_m.h>

#include "../../application/rdpapp/GenericAppMsgRdp_m.h"
#include "../common/L4ToolsRdp.h"
#include "../contract/rdp/RdpCommand_m.h"
#include "../rdp/rdp_common/RdpHeader_m.h"
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"
#include "RdpReceiveQueue.h"

namespace inet {

namespace rdp {

Estimator::Estimator(){

    flushCounter = 0;
}



void Estimator::setWindowSize(double _windowSize){
    windowSize = _windowSize;
}
void Estimator::addSample(double measurement, simtime_t timestamp){
    samples.emplace_back(measurement, timestamp);
    if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end() && (it->getTimestamp() + windowSize) < simTime())
        {
            // `erase()` invalidates the iterator, use returned iterator
            it = samples.erase(it);
     
         }
    }  
}

int Estimator::getFlushCounter(){
    return flushCounter;
}

void Estimator::flush(){
    flushCounter++;
    samples.clear();
}
unsigned int Estimator::getSize(){
    return samples.size();
}

double Estimator::getMean(){
    if (samples.empty()) {
        return 0;
    }
 
    double sum = 0.0;
    for (Measurement &i: samples) {
        sum += i.getMeasurement();
    }
    return sum / samples.size();
}


double Estimator::getMax(){
    if (samples.empty()) {
        return 0;
    }
 
    double _max = 0.0;
    for (Measurement &i: samples) {
        _max = std::max(_max, i.getMeasurement());
    }
    return _max;

}
double Estimator::getMin(){
     if (samples.empty()) {
        return 0;
    }
 
    double _min = 0.0;
    for (Measurement &i: samples) {
        if(_min == 0.0)
            _min = i.getMeasurement();

        _min = std::min(_min, i.getMeasurement());
    }
    return _min;
}

double Estimator::getStd(){

    if (samples.empty()) {
        return 0;
    }

    double mean = getMean();
    double variance = 0;

     for (Measurement &i: samples) {
        variance += pow(i.getMeasurement() - mean, 2);
    }

    return sqrt(variance / samples.size());
}

 double Estimator::getMean(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

     if (subsamples.empty()) {
        return 0;
    }
 
    double sum = 0.0;
    for (Measurement &i: subsamples) {
        sum += i.getMeasurement();
    }
    return sum / subsamples.size();


 }
 double Estimator::getMax(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

    if (subsamples.empty()) {
        return 0;
    }
 
    double _max = 0.0;
    for (Measurement &i: subsamples) {
        _max = std::max(_max, i.getMeasurement());
    }
    return _max;


 }
 double Estimator::getMin(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

     if (subsamples.empty()) {
        return 0;
    }
 
    double _min = 0.0;
    for (Measurement &i: subsamples) {
        if(_min == 0.0)
            _min = i.getMeasurement();

        _min = std::min(_min, i.getMeasurement());
    }
    return _min;


 }

 double Estimator::getStd(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

    if (subsamples.empty()) {
        return 0;
    }

    double mean = getMean(subwindow);
    double variance = 0;

     for (Measurement &i: subsamples) {
        variance += pow(i.getMeasurement() - mean, 2);
    }

    return sqrt(variance / subsamples.size());


 }

std::vector<Measurement> Estimator::getSamples(){
    return samples;
}

//
// helper functions
//

const char* RdpConnection::stateName(int state)
{
#define CASE(x)    case x: \
        s = #x + 5; break
    const char *s = "unknown";
    switch (state) {
        CASE(RDP_S_INIT)
;            CASE(RDP_S_CLOSED);
            CASE(RDP_S_LISTEN);
            CASE(RDP_S_ESTABLISHED);
        }
    return s;
#undef CASE
}

const char* RdpConnection::eventName(int event)
{
#define CASE(x)    case x: \
        s = #x + 5; break
    const char *s = "unknown";
    switch (event) {
        CASE(RDP_E_IGNORE)
;            CASE(RDP_E_OPEN_ACTIVE);
            CASE(RDP_E_OPEN_PASSIVE);
        }
    return s;
#undef CASE
}

const char* RdpConnection::indicationName(int code)
{
#define CASE(x)    case x: \
        s = #x + 5; break
    const char *s = "unknown";
    switch (code) {
        CASE(RDP_I_DATA);
        CASE(RDP_I_ESTABLISHED);
        CASE(RDP_I_PEER_CLOSED);

        }
    return s;
#undef CASE
}

void RdpConnection::sendToIP(Packet *packet, const Ptr<RdpHeader> &rdpseg)
{
    EV_TRACE << "RdpConnection::sendToIP" << endl;
    rdpseg->setSrcPort(localPort);
    rdpseg->setDestPort(remotePort);
    //EV_INFO << "Sending: " << endl;
    //printSegmentBrief(packet, rdpseg);
    IL3AddressType *addressType = remoteAddr.getAddressType();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    auto addresses = packet->addTagIfAbsent<L3AddressReq>();
    addresses->setSrcAddress(localAddr);
    addresses->setDestAddress(remoteAddr);

    //Set transmission time into packet header
    rdpseg->setTransmissionTime(simTime());

    insertTransportProtocolHeader(packet, Protocol::rdp, rdpseg);
    rdpMain->sendFromConn(packet, "ipOut");
}

void RdpConnection::sendToIP(Packet *packet, const Ptr<RdpHeader> &rdpseg, L3Address src, L3Address dest)
{
    //EV_INFO << "Sending: ";
    //printSegmentBrief(packet, rdpseg);
    IL3AddressType *addressType = dest.getAddressType();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    auto addresses = packet->addTagIfAbsent<L3AddressReq>();
    addresses->setSrcAddress(src);
    addresses->setDestAddress(dest);

    //Set transmission time into packet header
    rdpseg->setTransmissionTime(simTime());


    insertTransportProtocolHeader(packet, Protocol::rdp, rdpseg);
    rdpMain->sendFromConn(packet, "ipOut");
}

void RdpConnection::sendIndicationToApp(int code, const int id)
{
    EV_INFO << "Notifying app: " << indicationName(code) << endl;
    auto indication = new Indication(indicationName(code), code);
    RdpCommand *ind = new RdpCommand();
    ind->setNumRcvTrimmedHeader(state->numRcvTrimmedHeader);
    ind->setUserId(id);
    indication->addTag<SocketInd>()->setSocketId(socketId);
    indication->setControlInfo(ind);
    sendToApp(indication);
}

void RdpConnection::sendEstabIndicationToApp()
{
    EV_INFO << "Notifying app: " << indicationName(RDP_I_ESTABLISHED) << endl;
    auto indication = new Indication(indicationName(RDP_I_ESTABLISHED), RDP_I_ESTABLISHED);
    RdpConnectInfo *ind = new RdpConnectInfo();
    ind->setLocalAddr(localAddr);
    ind->setRemoteAddr(remoteAddr);
    ind->setLocalPort(localPort);
    ind->setRemotePort(remotePort);
    indication->addTag<SocketInd>()->setSocketId(socketId);
    indication->setControlInfo(ind);
    sendToApp(indication);
}

void RdpConnection::sendToApp(cMessage *msg)
{
    rdpMain->sendFromConn(msg, "appOut");
}

void RdpConnection::initConnection(RdpOpenCommand *openCmd)
{
    sendQueue = rdpMain->createSendQueue();
    sendQueue->setConnection(this);

    receiveQueue = rdpMain->createReceiveQueue();
    receiveQueue->setConnection(this);

    //create algorithm
    const char *rdpAlgorithmClass = openCmd->getRdpAlgorithmClass();

    if (!rdpAlgorithmClass || !rdpAlgorithmClass[0])
        rdpAlgorithmClass = rdpMain->par("rdpAlgorithmClass");

    rdpAlgorithm = check_and_cast<RdpAlgorithm*>(inet::utils::createOne(rdpAlgorithmClass));
    rdpAlgorithm->setConnection(this);
    // create state block
    state = rdpAlgorithm->getStateVariables();
    configureStateVariables();
    rdpAlgorithm->initialize();
}

void RdpConnection::configureStateVariables()
{
    state->IW = rdpMain->par("initialWindow");
    state->ssthresh = rdpMain->par("ssthresh");
    state->cwnd = state->IW;
    state->slowStartState = true;
    state->slowStartPacketsToSend = 0;
    state->sentPullsInWindow = state->IW;
    state->additiveIncreasePackets = rdpMain->par("additiveIncreasePackets");
    rdpMain->recordScalar("initialWindow=", state->IW);

}

// the receiver sends NACK when receiving a header
void RdpConnection::sendNackRdp(unsigned int nackNum)
{
    EV_INFO << "Sending Nack! NackNum: " << nackNum << endl;
    const auto &rdpseg = makeShared<RdpHeader>();
    rdpseg->setAckBit(false);
    rdpseg->setNackBit(true);
    rdpseg->setNackNo(nackNum);
    rdpseg->setSynBit(false);
    rdpseg->setIsDataPacket(false);
    rdpseg->setIsPullPacket(false);
    rdpseg->setIsHeader(false);
    std::string packetName = "RdpNack-" + std::to_string(nackNum);
    Packet *fp = new Packet(packetName.c_str());
    // send it
    sendToIP(fp, rdpseg);
}

void RdpConnection::sendAckRdp(unsigned int AckNum)
{
    EV_INFO << "Sending Ack! AckNum: " << AckNum << endl;
    const auto &rdpseg = makeShared<RdpHeader>();
    rdpseg->setAckBit(true);
    rdpseg->setAckNo(AckNum);
    rdpseg->setNackBit(false);
    rdpseg->setSynBit(false);
    rdpseg->setIsDataPacket(false);
    rdpseg->setIsPullPacket(false);
    rdpseg->setIsHeader(false);
    std::string packetName = "RdpAck-" + std::to_string(AckNum);
    Packet *fp = new Packet(packetName.c_str());
    // send it
    sendToIP(fp, rdpseg);
}

void RdpConnection::printConnBrief() const
{
    EV_DETAIL << "Connection " << localAddr << ":" << localPort << " to " << remoteAddr << ":" << remotePort << "  on socketId=" << socketId << "  in " << stateName(fsm.getState()) << endl;
}

void RdpConnection::printSegmentBrief(Packet *packet, const Ptr<const RdpHeader> &rdpseg)
{
    EV_STATICCONTEXT
    ;
    EV_INFO << "." << rdpseg->getSrcPort() << " > ";
    EV_INFO << "." << rdpseg->getDestPort() << ": ";

    if (rdpseg->getSynBit())
        EV_INFO << (rdpseg->getAckBit() ? "SYN+ACK " : "SYN ");
    if (rdpseg->getRstBit())
        EV_INFO << (rdpseg->getAckBit() ? "RST+ACK " : "RST ");
    if (rdpseg->getAckBit())
        EV_INFO << "ack " << rdpseg->getAckNo() << " ";
    EV_INFO << endl;
}

void RdpConnection::rttMeasurementComplete(simtime_t newRtt, bool isHeader){
    // update smoothed RTT estimate (srtt) and variance (rttvar)
    const double g = 0.125;    // 1 / 8; (1 - alpha) where alpha == 7 / 8;

    if(!isHeader){
        simtime_t errStep = newRtt - state->sRttStep; //Step srtt
        simtime_t err = newRtt - state->sRtt; // Global srtt
        
        if (state->sRtt == SIMTIME_ZERO){
            state->sRtt += err;
        }
        else{
            state->sRtt += g * err;
        }

        if (state->sRttStep == SIMTIME_ZERO){
            state->sRttStep +=errStep;
        }
        else{
            state->sRttStep += g * errStep;
        }

        if (state->rttvar == SIMTIME_ZERO){
            state->rttvar += g * (fabs(err));
        }else{
            state->rttvar += g * (fabs(err) - state->rttvar);
        }

        if (state->rttvarStep == SIMTIME_ZERO){
            state->rttvarStep += (fabs(errStep));
        }else{
            state->rttvarStep += g * (fabs(errStep) - state->rttvarStep);
        }

        state->latestRtt = newRtt;

        //Min RTT since start of the connection
        if(state->minRtt == SIMTIME_ZERO)
            state->minRtt = newRtt;
        else
            state->minRtt = std::min(state->minRtt, newRtt);

        //Min RTT in this step
        if(state->minRttStep == SIMTIME_ZERO)
            state->minRttStep = newRtt;
        else
            state->minRttStep = std::min(state->minRttStep, newRtt);
    }
    else{
        simtime_t errStep = newRtt - state->sRttStepHeader; //Step srtt
        simtime_t err = newRtt - state->sRttHeader; // Global srtt
        
        if (state->sRttHeader == SIMTIME_ZERO){
            state->sRttHeader += err;
        }
        else{
            state->sRttHeader += g * err;
        }

        if (state->sRttStepHeader == SIMTIME_ZERO){
            state->sRttStepHeader +=errStep;
        }
        else{
            state->sRttStepHeader += g * errStep;
        }

        if (state->rttvarHeader == SIMTIME_ZERO){
            state->rttvarHeader += g * (fabs(err));
        }else{
            state->rttvarHeader += g * (fabs(err) - state->rttvarHeader);
        }

        if (state->rttvarStepHeader == SIMTIME_ZERO){
            state->rttvarStepHeader += (fabs(errStep));
        }else{
            state->rttvarStepHeader += g * (fabs(errStep) - state->rttvarStepHeader);
        }

        state->latestRttHeader = newRtt;

        //Min RTT since start of the connection
        if(state->minRttHeader == SIMTIME_ZERO)
            state->minRttHeader = newRtt;
        else
            state->minRttHeader = std::min(state->minRttHeader, newRtt);

        //Min RTT in this step
        if(state->minRttStepHeader == SIMTIME_ZERO)
            state->minRttStepHeader = newRtt;
        else
            state->minRttStepHeader = std::min(state->minRttStepHeader, newRtt);
    }
}

void RdpConnection::computeRtt(unsigned int pullSeqNum, bool isHeader){
    if (state->pullRequestsTransmissionTimes.find(pullSeqNum) != state->pullRequestsTransmissionTimes.end()){
        simtime_t rtt = simTime() - state->pullRequestsTransmissionTimes[pullSeqNum];
        if(state->rttPropEstimator.getSize() == 1 && state->rttPropEstimator.getFlushCounter() == 0)
            state->rttPropEstimator.flush();

        state->rttPropEstimator.addSample(rtt.dbl(), simTime());
        state->pullRequestsTransmissionTimes.erase(pullSeqNum);
        rttMeasurementComplete(rtt, isHeader);
    }
}

uint32 RdpConnection::convertSimtimeToTS(simtime_t simtime)
{
    ASSERT(SimTime::getScaleExp() <= -3);
    uint32 timestamp = (uint32) (simtime.inUnit(SIMTIME_MS));
    return timestamp;
}

simtime_t RdpConnection::convertTSToSimtime(uint32 timestamp)
{
    ASSERT(SimTime::getScaleExp() <= -3);
    simtime_t simtime(timestamp, SIMTIME_MS);
    return simtime;
}

void RdpConnection::cancelRequestTimer(){
    if(paceTimerMsg->isScheduled()){
        cancelEvent(paceTimerMsg);

    }
}

void RdpConnection::paceChanged(double newPace){
        // Set the new pacing timer. At this point we should probably cancel the 
        // next timer and reschedule it, because the transmission rate has decreased
        // or increased.
        // Let sendingTime be the time in which the last PR was sent. The newArrivalTime will be now
        // if sendingTime + newPace <= simTime(), else sendingTime + newPace.
        Enter_Method("paceChanged");
        if(paceTimerMsg->isScheduled()){
            simtime_t sendingTime = paceTimerMsg->getSendingTime();
            simtime_t newArrivalTime;
            if (sendingTime + newPace <= simTime()){
                newArrivalTime = simTime();
            }else{
                newArrivalTime = sendingTime + newPace;
            }

            cancelEvent(paceTimerMsg);
            take(paceTimerMsg);
            scheduleAt(newArrivalTime, paceTimerMsg);
        }
}


} // namespace rdp

} // namespace inet

