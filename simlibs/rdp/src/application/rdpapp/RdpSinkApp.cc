#include <inet/common/lifecycle/ModuleOperations.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/ProtocolTag_m.h>
#include <inet/common/lifecycle/NodeStatus.h>
#include <inet/common/packet/Message.h>
#include <inet/common/ResultFilters.h>

#include "../../transportlayer/contract/rdp/RdpCommand_m.h"
#include "GenericAppMsgRdp_m.h"
#include "RdpSinkApp.h"

namespace inet {
#define SEND_INIT_REQUEST_TO_READ    0

Define_Module(RdpSinkApp);

simsignal_t RdpSinkApp::rcvdPkSignalNDP = registerSignal("packetReceived");

simsignal_t goodputSigNdp = NodeStatus::registerSignal("goodputSigNdp");  //data that is delivered - trimmed packets/lost should be ignored
simsignal_t fctRecordv3 = NodeStatus::registerSignal("fctRecordv3");
simsignal_t numRcvTrimmedHeaderSigNdp = NodeStatus::registerSignal("numRcvTrimmedHeaderSigNdp");

void RdpSinkApp::initialize(int stage)
{
    EV_TRACE << "RdpSinkApp::initialize";
    cSimpleModule::initialize(stage);
    recordStatistics = par("recordStatistics");
    if (stage == INITSTAGE_LOCAL) {
        bytesRcvd = 0;
        WATCH(bytesRcvd);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus*>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");
        const char *localAddress = par("localAddress");
        int localPort = par("localPort");
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localAddress[0] ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
        socket.listen();
    }
}

void RdpSinkApp::handleMessage(cMessage *msg)
{
    if (msg->getKind() == RDP_I_PEER_CLOSED) {
        EV_TRACE << "RdpSinkApp:handleMessage Message RDP_I_PEER_CLOSED" << endl;
        if(recordStatistics == true){
            EV_INFO << "RDP_I_PEER_CLOSED message arrived - end of RDP connection" << endl;
            tEndAdded = simTime();

            RdpCommand *controlInfo = check_and_cast<RdpCommand*>(msg->getControlInfo());
            numRcvTrimmedHeader = controlInfo->getNumRcvTrimmedHeader();
            cModule *centralMod =  this->getParentModule()->getModuleByPath("centralScheduler");
            if (centralMod && recordStatistics == true) {
                int numFinishedFlows = centralMod->par("numCompletedShortFlows");
                int newNumFinishedFlows = numFinishedFlows + 1;
                centralMod->par("numCompletedShortFlows").setIntValue(newNumFinishedFlows);
                EV_INFO << "RdpSinkApp::handleMessage  numCompletedShortFlows " << newNumFinishedFlows << endl;
            }
        }
        delete msg;
    }
    else if (msg->getKind() == RDP_I_DATA) {
        EV_TRACE << "RdpSinkApp:handleMessage Message RDP_I_DATA" << endl;
        if(recordStatistics == true){
            Packet *packet = check_and_cast<Packet*>(msg);
            bytesRcvd += packet->getByteLength();
            EV_INFO << "RDP DATA message arrived - bytesRcvd: " << bytesRcvd << endl;
            emit(rcvdPkSignalNDP, packet);
            // Moh added: time stamp when receiving the first data packet (not the SYN, as the app wouldn't get that packet)
            if (firstDataReceived == true) {
                tStartAdded = packet->getTag<CreationTimeTag>()->getCreationTime();
                firstDataReceived = false;
            }
            else{
                simtime_t startTime = packet->getTag<CreationTimeTag>()->getCreationTime();
                if(startTime < tStartAdded){
                    tStartAdded = startTime;
                }
            }
            EV_INFO << "Sink Application bytes received: " << bytesRcvd << " " << this->getFullPath() << std::endl;
        }
        else{
            //IGNORE
        }
    }
    else if (msg->getKind() == RDP_I_ESTABLISHED) {
        EV_TRACE << "RdpSinkApp:handleMessage Message RDP_I_ESTABLISHED" << endl;
        EV_INFO << "RDP_I_ESTABLISHED message arrived - deleting message" << endl;
        delete msg;
    }
    else{
        EV_TRACE << "RdpSinkApp:handleMessage Message UNKNOWN" << endl;
        EV_WARN << "Unknown Message Type Arrived at Sink App" << endl;
        delete msg;
        // must be data or some kind of indication -- can be dropped
        }
}

void RdpSinkApp::finish()
{
    EV_TRACE << "RdpSinkApp::finish" << endl;
    double throughput = 8 * (double) bytesRcvd / (tEndAdded - tStartAdded).dbl();
    EV_INFO << "Time Difference: " << (tEndAdded - tStartAdded).dbl() << endl;
    double FCT = SIMTIME_DBL(tEndAdded - tStartAdded);
    EV_INFO << "Flow Completion Time:=    " << FCT << endl;
    // don't emit the FCT of the background flows(no need), we just observe the shorter length flows

    std::cout << "Bytes Received: " << bytesRcvd << endl;
    std::cout << "Time Start: " << tStartAdded << endl;
    std::cout << "Time End: " << tEndAdded << endl;
    std::cout << "Throughput: " << throughput << endl;
    if (recordStatistics == true) {
        emit(fctRecordv3, FCT);
        emit(goodputSigNdp, throughput);
        emit(numRcvTrimmedHeaderSigNdp, numRcvTrimmedHeader);
        EV_INFO << "RdpSinkApp::finish() bytesRcvd " << endl;
        EV_INFO << "bytesRcvd: " << bytesRcvd << "  " << this->getFullPath() << endl;

        std::cout << "RdpSinkApp::finish() bytesRcvd " << endl;
        std::cout << "bytesRcvd: " << bytesRcvd << "  " << this->getFullPath() << endl;
    }
}

void RdpSinkApp::refreshDisplay() const
{
    std::ostringstream os;
    getDisplayString().setTagArg("t", 0, os.str().c_str());
}

} // namespace inet
