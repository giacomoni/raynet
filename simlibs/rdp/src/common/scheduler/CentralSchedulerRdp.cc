#include "CentralSchedulerRdp.h"

using namespace std;
using namespace std::chrono;

namespace inet {

Define_Module(CentralSchedulerRdp);

CentralSchedulerRdp::~CentralSchedulerRdp()
{
    cancelAndDelete(startManagerNode);
    cancelAndDelete(stopSimulation);

}

void CentralSchedulerRdp::initialize(int stage)
{
    myfile.open("aaaaaaa.csv");
    isWebSearchWorkLoad = par("isWebSearchWorkLoad");
    indexWorkLoad = 0;

    // Record start time
    t1 = high_resolution_clock::now();
//    omnetpp::envir::Stopwatch stopwatch;
//    stopwatch.setRealTimeLimit(5);
    numCompletedShortFlows = par("numCompletedShortFlows");
    WATCH(numCompletedShortFlows);

    IW = par("IW");
    rdpSwitchQueueLength = par("rdpSwitchQueueLength");
    perFlowEcmp = par("perFlowEcmp");
    perPacketEcmp = par("perPacketEcmp");

    randMapShortFlowsVec.setName("randMapShortFlowsVec");
    permMapShortFlowsVector.setName("permMapShortFlowsVector");

    permMapLongFlowsVec.setName("permMapLongFlowsVec");
    permMapShortFlowsVec.setName("permMapShortFlowsVec");
    numTcpSessionAppsVec.setName("numTcpSessionAppsVec");
    numTcpSinkAppsVec.setName("numTcpSinkAppsVec");
    matSrc.setName("matSrc");
    matDest.setName("matDest");
    nodes.setName("nodes");

    std::cout << "\n\n Central flow scheduler \n";
    kValue = par("kValue");
    trafficMatrixType = par("trafficMatrixType");
    arrivalRate = par("arrivalRate");

    // seed vale and rng
    seedValue = par("seedValue");
    srand(seedValue);   //  srand(time(NULL));
    PRNG = std::mt19937(seedValue);
    expDistribution = std::exponential_distribution<double>(arrivalRate);

    flowSize = par("flowSize");
    numShortFlows = par("numShortFlows");
    longFlowSize = par("longFlowSize");
    numServers = std::pow(kValue, 3) / 4;
    shuffle = par("shuffle");

    percentLongFlowNodes = par("percentLongFlowNodes");
    numIncastSenders = par("numIncastSenders");
    oneToOne = par("oneToOne");
    daisyChainGFS = par("daisyChainGFS");
    // multicast variables multiSourcing
    oneToMany = par("oneToMany");
    numRunningMulticastGroups = par("numRunningMulticastGroups");
    manyToOne = par("manyToOne");
    numRunningMultiSourcingGroups = par("numRunningMultiSourcingGroups");

    numReplica = par("numReplica");
    std::cout << " =====  SIMULATION CONFIGURATIONS ========= " << "\n";
    std::cout << " =====  numServers   : " << numServers << "         ========= \n";
    std::cout << " =====  ShortflowSize: " << flowSize << "      ========= \n";
    std::cout << " =====  numShortFlows: " << numShortFlows << "          ========= \n";
    std::cout << " =====  arrivalRate  : " << arrivalRate << "       ========= \n";
    std::cout << " ========================================== " << "\n";
    stopSimulation = new cMessage("stopSimulation");

    startManagerNode = new cMessage("startManagerNode");
    scheduleAt(0.0, startManagerNode);
}

void CentralSchedulerRdp::handleMessage(cMessage *msg)
{
    if (msg == stopSimulation) {
        std::cout << " All shortFlows COMPLETED  " << std::endl;
        totalSimTime = simTime();
        endSimulation();
    }

    std::cout << "******************** CentralSchedulerRdp::handleMessage .. ********************  \n";

    //  % of short flows and % of long flows
//    numlongflowsRunningServers = floor(numServers * 0.33); // 33% of nodes run long flows
    numlongflowsRunningServers = floor(numServers * percentLongFlowNodes); // 33% of nodes run long flows , TODO throw error as it shouldn't be 1
    numshortflowRunningServers = numServers - numlongflowsRunningServers;
    std::cout << "numshortflowRunningServers:  " << numshortflowRunningServers << std::endl;
    std::cout << "numlongflowsRunningServers:  " << numlongflowsRunningServers << std::endl;

    generateTM();
    serversLocations();

    std::string itsSrc;
    std::string newDest;

    //deleteAllSubModuleApp("app[0]"); //ndpApp to app
    deleteAllSubModuleApp("app[0]");
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    std::cout << "\n\n ******************** schedule Incast  .. ********************  \n";
    //    scheduleIncast(numIncastSenders);
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    std::cout << "\n\n ******************** schedule Long flows .. ********************  \n";
    scheduleLongFlows();

    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    if (oneToMany == true && manyToOne == false && oneToOne == false && daisyChainGFS == false) {
        for (int i = 1; i <= numRunningMulticastGroups; i++) {
            std::cout << "\n\n\n multicast Group ID: " << i << std::endl;
            std::vector<std::string> replicasNodes;
            getNewThreeDestRandTMForMulticast(itsSrc, replicasNodes); // ndp multicasting (3 replicas) based on multi Unicasting
            scheduleNewMultiCastSession(itsSrc, replicasNodes, i);
        }
    }
    ////// daisyChainGFS  multicast
    else if (oneToMany == false && manyToOne == false && oneToOne == false && daisyChainGFS == true) {
        for (int i = 1; i <= numRunningMulticastGroups; i++) {
            std::cout << "\n\n\n daisyChainGFS ::: multicast Group ID: " << i << std::endl;
            std::vector<std::string> replicasNodes;
            getNewThreeDestRandTMForMulticast(itsSrc, replicasNodes); //   (3 replicas)
            scheduleNewDaisyChainSession(itsSrc, replicasNodes, i);
        }
    }
    ////////// ^^^^^^^
    else if (oneToMany == false && manyToOne == true && oneToOne == false && daisyChainGFS == false) {
        for (int i = 1; i <= numRunningMultiSourcingGroups; i++) {
            std::cout << "\n\n\n multi-sourcing Group ID: " << i << std::endl;
            std::vector<std::string> senders;
            getNewThreeSrcRandTMForMultiSourcing(newDest, senders); // ndp multisourcing (3 replicas) based on multi Unicasting
            scheduleNewMultiSourcingSession(newDest, senders, i);
        }
    }
    ////////
    else if (oneToMany == false && manyToOne == false && oneToOne == true && daisyChainGFS == false) {
        if (isWebSearchWorkLoad == true)
            getWebSearchWorkLoad();

        for (int i = 1; i <= numShortFlows; i++) {
            std::cout << "\n\n ******************** schedule Short flows .. ********************  \n";
            std::cout << " Shortflow ID: " << i << std::endl;
            //std::cout << " Source: " << itsSrc << std::endl;
            //std::cout << " Dest: " << newDest << std::endl;
            if (strcmp(trafficMatrixType, "randTM") == 0){
                getNewDestRandTM(itsSrc, newDest);
            }
            else if (strcmp(trafficMatrixType, "permTM") == 0){
                getNewDestPremTM(itsSrc, newDest);
            }
            std::cout << "\n\n\nSCHEDULING SHORT FLOW: " << itsSrc << newDest;
            scheduleNewShortFlow(itsSrc, newDest);
        }
    }
    std::cout << "\n\nCentral Scheduler complete!" << std::endl;
    std::cout << "\n\n\n";

}

void CentralSchedulerRdp::serversLocations()
{
    std::cout << "\n\n ******************** serversLocations .. ********************  \n";
    int serversPerPod = pow(kValue, 2) / 4;
    int serversPerRack = kValue / 2;

    for (int m = 0; m < numServers; m++) {
        NodeLocation nodeLocation;
        nodeLocation.index = permServers.at(m);
        nodeLocation.pod = floor(permServers.at(m) / serversPerPod);
        nodeLocation.rack = floor((permServers.at(m) % serversPerPod) / serversPerRack);
        nodeLocation.node = permServers.at(m) % serversPerRack;
        nodeLocation.numTCPSink = 0;
        nodeLocation.numTCPSession = 0;
        nodeLocationList.push_back(nodeLocation);
    }

    std::list<NodeLocation>::iterator it;
    it = nodeLocationList.begin();
    while (it != nodeLocationList.end()) {
        std::cout << " index: " << it->index << " ==> " << " [pod, rack, node] =   ";
        std::cout << " [" << it->pod << ", " << it->rack << ", " << it->node << "] \n";
        it++;
    }
}

// random TM
void CentralSchedulerRdp::getNewDestRandTM(std::string &itsSrc, std::string &newDest)
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << "******************** getNewDestination RandTM .. ********************  \n";
//    int newDestination = test;
    int newDestination = 0;
    int srcNewDestination = 0;
    while (newDestination == srcNewDestination) { // the dest should be different from the src
        newDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
        //    int srcNewDestination = permMapShortFlows.find(newDestination)->second; // this line is used wit premTM not randTM
        srcNewDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
    }
    std::cout << "@@@ newDestination " << newDestination << " , its src   " << srcNewDestination << "\n";

    CentralSchedulerRdp::findLocation(newDestination, newDest);
    CentralSchedulerRdp::findLocation(srcNewDestination, itsSrc);

    RecordMat recordMat;
    recordMat.recordSrc = srcNewDestination;
    recordMat.recordDest = newDestination;
    recordMatList.push_back(recordMat);

    // can be replaced by recordMatList ( see Finish())
    randMapShortFlowsVec.record(srcNewDestination);
    randMapShortFlowsVec.record(newDestination);
}

// permutation TM
void CentralSchedulerRdp::generateTM()
{
//    std::random_device rd; // uniformly-distributed integer random number generator-- seed
//    std::mt19937 rng(rd());
    std::cout << "\n\n ******************** generate TM maps.. ********************  \n";
    for (int i = 0; i < numServers; ++i)
        permServers.push_back(i);
    if (shuffle)
        std::shuffle(permServers.begin(), permServers.end(), PRNG); ///////////// TODO ooooooooooooooooooo

    for (int i = 0; i < numServers; ++i) {
        if (i < numlongflowsRunningServers) {
            permLongFlowsServers.push_back(permServers.at(i));
            permMapLongFlows.insert(std::pair<int, int>(permServers.at((i + 1) % numlongflowsRunningServers), permServers.at(i)));  // < dest, src >
        }
        else if (i >= numlongflowsRunningServers && i < numServers - 1) {
            permShortFlowsServers.push_back(permServers.at(i));
            permMapShortFlows.insert(std::pair<int, int>(permServers.at(i + 1), permServers.at(i)));  // < dest, src >
        }
        else if (i == numServers - 1) {
//            permShortFlowsServers.push_back(permServers.at(numlongflowsRunningServers));
            permShortFlowsServers.push_back(permServers.at(i));
            permMapShortFlows.insert(std::pair<int, int>(permServers.at(numlongflowsRunningServers), permServers.at(i)));  // < dest, src >
        }
    }

    std::cout << "permServers:                ";
    for (std::vector<int>::iterator it = permServers.begin(); it != permServers.end(); ++it)
        std::cout << ' ' << *it;
    std::cout << '\n';

    std::cout << "permLongFlowsServers:       ";
    for (std::vector<int>::iterator it = permLongFlowsServers.begin(); it != permLongFlowsServers.end(); ++it)
        std::cout << ' ' << *it;
    std::cout << '\n';

    std::cout << "permShortFlowsServers:      ";
    for (std::vector<int>::iterator it = permShortFlowsServers.begin(); it != permShortFlowsServers.end(); ++it)
        std::cout << ' ' << *it;
    std::cout << '\n';

    std::cout << "permMapLongFlows:                 \n";
    for (std::map<int, int>::iterator iter = permMapLongFlows.begin(); iter != permMapLongFlows.end(); ++iter) {
        cout << "  src " << iter->second << " ==> ";
        cout << "  dest " << iter->first << "\n";
        permMapLongFlowsVec.record(iter->second);
        permMapLongFlowsVec.record(iter->first);
    }

    std::cout << "permMapShortFlows:                 \n";
    for (std::map<int, int>::iterator iter = permMapShortFlows.begin(); iter != permMapShortFlows.end(); ++iter) {
        cout << "   src " << iter->second << " ==> ";
        cout << "   dest " << iter->first << "\n";
        permMapShortFlowsVec.record(iter->second);
        permMapShortFlowsVec.record(iter->first);
    }

}

void CentralSchedulerRdp::getNewDestPremTM(std::string &itsSrc, std::string &newDest)
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << "******************** getNewDestination PremTM .. ********************  \n";
    int newDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));

    int srcNewDestination = permMapShortFlows.find(newDestination)->second;
    std::cout << "@@@ newDestination " << newDestination << " , its src   " << srcNewDestination << "\n";

    CentralSchedulerRdp::findLocation(newDestination, newDest);
    CentralSchedulerRdp::findLocation(srcNewDestination, itsSrc);
//    std::cout << " newDest " << newDest << "\n";
//    std::cout << " itsSrc " << itsSrc << "\n";
    RecordMat recordMat;
    recordMat.recordSrc = srcNewDestination;
    recordMat.recordDest = newDestination;
    recordMatList.push_back(recordMat);

    permMapShortFlowsVector.record(srcNewDestination);
    permMapShortFlowsVector.record(newDestination);
}

void CentralSchedulerRdp::findLocation(int nodeIndex, std::string &nodePodRackLoc)
{
    std::list<NodeLocation>::iterator itt;
    itt = nodeLocationList.begin();
    while (itt != nodeLocationList.end()) {
        if (itt->index == nodeIndex) {
            nodePodRackLoc = "FatTreeRdp.Pod[" + std::to_string(itt->pod) + "].racks[" + std::to_string(itt->rack) + "].servers[" + std::to_string(itt->node) + "]";
        }
        itt++;
    }
}

void CentralSchedulerRdp::scheduleLongFlows()
{
    std::cout << "\n\n ******************** scheduleLongFlows .. ********************  \n";
    std::string dest;
    std::string source;

// iterate permMapLongFlows
    for (std::map<int, int>::iterator iter = permMapLongFlows.begin(); iter != permMapLongFlows.end(); ++iter) {
        cout << "\n\n NEW LONGFLOW :)   ";
        cout << "  host(SRC.)= " << iter->second << " ==> " << "  host(DEST.)= " << iter->first << "\n";

        RecordMat recordMat;
        recordMat.recordSrc = iter->second;
        recordMat.recordDest = iter->first;
        recordMatList.push_back(recordMat);

        CentralSchedulerRdp::findLocation(iter->first, dest);
        CentralSchedulerRdp::findLocation(iter->second, source);
        cout << "  nodePodRackLoc:  " << iter->second << " == " << source << " ==> " << iter->first << " == " << dest << "\n";

        cModule *srcModule = getModuleByPath(source.c_str());
        cModule *destModule = getModuleByPath(dest.c_str());

        // Receiver sink app
        cModule *rdpDestModule = destModule->getSubmodule("at");
        int newRdpGateOutSizeDest = rdpDestModule->gateSize("out") + 1;
        int newRdpGateInSizeDest = rdpDestModule->gateSize("in") + 1;
        rdpDestModule->setGateSize("out", newRdpGateOutSizeDest);
        rdpDestModule->setGateSize("in", newRdpGateInSizeDest);
        int newNumRdpSinkAppsDest = findNumSumbodules(destModule, "rdp.application.rdpapp.RdpSinkApp") + findNumSumbodules(destModule, "rdp.application.rdpapp.RdpBasicClientApp") + 1;
        cModuleType *moduleTypeDest = cModuleType::get("rdp.application.rdpapp.RdpSinkApp");
        std::string nameRdpAppDest = "app[" + std::to_string(newNumRdpSinkAppsDest - 1) + "]";
        cModule *newDestAppModule = moduleTypeDest->create(nameRdpAppDest.c_str(), destModule);
        newDestAppModule->par("localAddress").setStringValue(dest.c_str());
        newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);
        newDestAppModule->par("recordStatistics").setBoolValue(false);
        //   --------<ndpIn         appOut[]<----------
        //     ndpApp                          ndp
        //   -------->ndpOut        appIn[] >----------
        cGate *gateRdpInDest = rdpDestModule->gate("in", newRdpGateOutSizeDest - 1);
        cGate *gateRdpOutDest = rdpDestModule->gate("out", newRdpGateOutSizeDest - 1);
        cGate *gateInDest = newDestAppModule->gate("socketIn");
        cGate *gateOutDest = newDestAppModule->gate("socketOut");
        gateRdpOutDest->connectTo(gateInDest);
        gateOutDest->connectTo(gateRdpInDest);
        newDestAppModule->finalizeParameters();
        newDestAppModule->buildInside();
        newDestAppModule->scheduleStart(simTime());
        newDestAppModule->callInitialize();
        newDestAppModule->par("localAddress").setStringValue(dest.c_str());
        newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);
        newDestAppModule->par("recordStatistics").setBoolValue(false);

        // Sender  app
        cModule *rdpSrcModule = srcModule->getSubmodule("at");
        int newRDPGateOutSizeSrc = rdpSrcModule->gateSize("out") + 1;
        int newRDPGateInSizeSrc = rdpSrcModule->gateSize("in") + 1;
        rdpSrcModule->setGateSize("out", newRDPGateOutSizeSrc);
        rdpSrcModule->setGateSize("in", newRDPGateInSizeSrc);
        int newNumRdpSessionAppsSrc = findNumSumbodules(srcModule, "ndp.application.ndpapp.RdpBasicClientApp") + findNumSumbodules(srcModule, "rdp.application.rdpapp.RdpSinkApp") + 1;
        cModuleType *moduleTypeSrc = cModuleType::get("ndp.application.ndpapp.RdpBasicClientApp");
        std::string nameRdpAppSrc = "app[" + std::to_string(newNumRdpSessionAppsSrc - 1) + "]";
        cModule *newSrcAppModule = moduleTypeSrc->create(nameRdpAppSrc.c_str(), srcModule);
        newSrcAppModule->par("localAddress").setStringValue(source.c_str());
        newSrcAppModule->par("connectAddress").setStringValue(dest.c_str());
        newSrcAppModule->par("connectPort").setIntValue(80 + newNumRdpSinkAppsDest);
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl());
        // >>>>>>
        newSrcAppModule->par("numPacketsToSend").setIntValue(longFlowSize); // should be longFlowSize
        //   --------<ndpIn         appOut[]<----------
        //     ndpApp                          ndp
        //   -------->ndpOut        appIn[] >----------
        cGate *gateRdpIn = rdpSrcModule->gate("in", newRDPGateInSizeSrc - 1);
        cGate *gateRdpOut = rdpSrcModule->gate("out", newRDPGateOutSizeSrc - 1);
        cGate *gateIn = newSrcAppModule->gate("socketIn");
        cGate *gateOut = newSrcAppModule->gate("socketOut");
        gateRdpOut->connectTo(gateIn);
        gateOut->connectTo(gateRdpIn);
        newSrcAppModule->finalizeParameters();
        newSrcAppModule->buildInside();
        newSrcAppModule->scheduleStart(simTime());
        newSrcAppModule->callInitialize(); //check all par parameters - got from manual
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl());
    }
}

void CentralSchedulerRdp::scheduleIncast(int numSenders)
{

    std::string itsSrc;
    std::string newDest;
    int newDestination = 0;
    CentralSchedulerRdp::findLocation(newDestination, newDest);

    sumArrivalTimes = 0;

    // get the src ndp module
    cModule *destModule = getModuleByPath(newDest.c_str());
    cModule *rdpDestModule = destModule->getSubmodule("at");
    int newRdpGateOutSizeDest = rdpDestModule->gateSize("out") + 1;
    int newRdpGateInSizeDest = rdpDestModule->gateSize("in") + 1;
    rdpDestModule->setGateSize("out", newRdpGateOutSizeDest);
    rdpDestModule->setGateSize("in", newRdpGateInSizeDest);
    int newNumRdpSinkAppsDest = findNumSumbodules(destModule, "rdp.application.rdpapp.RdpSinkApp") + 1;
    cModuleType *moduleTypeDest = cModuleType::get("rdp.application.rdpapp.RdpSinkApp");
    std::string nameRdpAppDest = "app[" + std::to_string(newNumRdpSinkAppsDest - 1) + "]";
    cModule *newDestAppModule = moduleTypeDest->create(nameRdpAppDest.c_str(), destModule);
    newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);

    cGate *gateRdpInDest = rdpDestModule->gate("in", newRdpGateOutSizeDest - 1);
    cGate *gateRdpOutDest = rdpDestModule->gate("out", newRdpGateOutSizeDest - 1);
    cGate *gateInDest = newDestAppModule->gate("socketIn");
    cGate *gateOutDest = newDestAppModule->gate("socketOut");
    gateRdpOutDest->connectTo(gateInDest);
    gateOutDest->connectTo(gateRdpInDest);
    newDestAppModule->finalizeParameters();
    newDestAppModule->buildInside();
    newDestAppModule->scheduleStart(simTime());
    newDestAppModule->callInitialize();

    for (int i = 0; i < numSenders; ++i) {

        int srcNewDestination = 0;
        while (newDestination == srcNewDestination) {
            srcNewDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
        }

        CentralSchedulerRdp::findLocation(srcNewDestination, itsSrc);

        cModule *srcModule = getModuleByPath(itsSrc.c_str()); // const char* c_str Return pointer to the string.
        cModule *rdpSrcModule = srcModule->getSubmodule("at");
        int newRDPGateOutSizeSrc = rdpSrcModule->gateSize("out") + 1;
        int newRDPGateInSizeSrc = rdpSrcModule->gateSize("in") + 1;
        rdpSrcModule->setGateSize("out", newRDPGateOutSizeSrc);
        rdpSrcModule->setGateSize("in", newRDPGateInSizeSrc);
        int newNumRdpSessionAppsSrc = findNumSumbodules(srcModule, "ndp.application.ndpapp.RdpBasicClientApp") + 1;
        cModuleType *moduleTypeSrc = cModuleType::get("ndp.application.ndpapp.RdpBasicClientApp");
        std::string nameRdpAppSrc = "app[" + std::to_string(newNumRdpSessionAppsSrc - 1) + "]";
        cModule *newSrcAppModule = moduleTypeSrc->create(nameRdpAppSrc.c_str(), srcModule);
        newSrcAppModule->par("connectAddress").setStringValue(newDest);
        newSrcAppModule->par("connectPort").setIntValue(80 + newNumRdpSinkAppsDest);  //??? to be checked
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);
        newSrcAppModule->par("numPacketsToSend").setIntValue(flowSize); //

        cGate *gateRdpIn = rdpSrcModule->gate("in", newRDPGateInSizeSrc - 1);
        cGate *gateRdpOut = rdpSrcModule->gate("out", newRDPGateOutSizeSrc - 1);
        cGate *gateIn = newSrcAppModule->gate("socketIn");
        cGate *gateOut = newSrcAppModule->gate("socketOut");
        gateRdpOut->connectTo(gateIn);
        gateOut->connectTo(gateRdpIn);
        newSrcAppModule->finalizeParameters();
        newSrcAppModule->buildInside();
        newSrcAppModule->scheduleStart(simTime());
        newSrcAppModule->callInitialize();
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);
    }
}

void CentralSchedulerRdp::scheduleNewShortFlow(std::string itsSrc, std::string newDest)
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@ scheduleNewShortFlow .. @@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << " newDest " << newDest << "\n";
    std::cout << " itsSrc " << itsSrc << "\n";

    newArrivalTime = expDistribution.operator()(PRNG);
    sumArrivalTimes = sumArrivalTimes + newArrivalTime;
    std::cout << " newArrivalTime = " << newArrivalTime << "\n\n";
    cModule *srcModule = getModuleByPath(itsSrc.c_str());
    cModule *destModule = getModuleByPath(newDest.c_str());
    std::cout << "srcModule:  " << srcModule->getFullPath() << "  , destModule:  " << destModule->getFullPath() << std::endl << std::endl;

    // sink app
    auto rdpDestModule = destModule->getSubmodule("at"); //FatTreeRdp.Pod[3].racks[0].servers[0]
    //dispatcher->setGateSize("in", dispatcher->gateSize("in") + 1);
    int newRdpGateOutSizeDest = rdpDestModule->gateSize("out") + 1;
    int newRdpGateInSizeDest = rdpDestModule->gateSize("in") + 1;
    rdpDestModule->setGateSize("out", newRdpGateOutSizeDest);
    rdpDestModule->setGateSize("in", newRdpGateInSizeDest);
    int newNumRdpSinkAppsDest = findNumSumbodules(destModule, "rdp.application.rdpapp.RdpSinkApp") + findNumSumbodules(destModule, "ndp.application.ndpapp.RdpBasicClientApp") + 1;
    // find factory object
    cModuleType *moduleTypeDest = cModuleType::get("rdp.application.rdpapp.RdpSinkApp");
    std::string nameRdpAppDest = "app[" + std::to_string(newNumRdpSinkAppsDest - 1) + "]";
    // create module and build its submodules (if any)
    cModule *newDestAppModule = moduleTypeDest->create(nameRdpAppDest.c_str(), destModule);
    newDestAppModule->par("localAddress").setStringValue(newDest);
    newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);

    //   --------<ndpIn         ndpOut[]<----------
    //     ndpApp                          ndp
    //   -------->ndpOut        appIn[] >----------

    //std::cout << "\n" << "DEST MODULE INFO";
    //std::cout << "\n NDP DEST" << rdpDestModule->getFullName();
    //std::cout << "\n NDP DEST APP" << newDestAppModule->getFullName();
    //std::cout << "\n" << rdpDestModule->getGateNames();
    cGate *gateRdpInDest = rdpDestModule->gate("in", newRdpGateInSizeDest - 1);
    cGate *gateRdpOutDest = rdpDestModule->gate("out", newRdpGateOutSizeDest - 1);
    cGate *gateInDest = newDestAppModule->gate("socketIn");
    cGate *gateOutDest = newDestAppModule->gate("socketOut");
    gateRdpOutDest->connectTo(gateInDest);
    gateOutDest->connectTo(gateRdpInDest);
    newDestAppModule->finalizeParameters();
    newDestAppModule->buildInside();
    newDestAppModule->scheduleStart(simTime());
    newDestAppModule->callInitialize();

    // src app
    cModule *rdpSrcModule = srcModule->getSubmodule("at");
    int newRDPGateOutSizeSrc = rdpSrcModule->gateSize("out") + 1;
    int newRDPGateInSizeSrc = rdpSrcModule->gateSize("in") + 1;
    rdpSrcModule->setGateSize("out", newRDPGateOutSizeSrc);
    rdpSrcModule->setGateSize("in", newRDPGateInSizeSrc);
    int newNumRdpSessionAppsSrc = findNumSumbodules(srcModule, "ndp.application.ndpapp.RdpBasicClientApp") + findNumSumbodules(srcModule, "rdp.application.rdpapp.RdpSinkApp") + 1;
    // find factory object
    cModuleType *moduleTypeSrc = cModuleType::get("ndp.application.ndpapp.RdpBasicClientApp");
    std::string nameRdpAppSrc = "app[" + std::to_string(newNumRdpSessionAppsSrc - 1) + "]";
    cModule *newSrcAppModule = moduleTypeSrc->create(nameRdpAppSrc.c_str(), srcModule);

    newSrcAppModule->par("localAddress").setStringValue(itsSrc);
    newSrcAppModule->par("connectAddress").setStringValue(newDest);
    newSrcAppModule->par("connectPort").setIntValue(80 + newNumRdpSinkAppsDest);
    newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);  //TODO
    // >>>>>>
    if (isWebSearchWorkLoad == false) {
        newSrcAppModule->par("numPacketsToSend").setIntValue(flowSize); //
    }
    else if (isWebSearchWorkLoad == true) {
        int newFlowSize = getNewFlowSizeFromWebSearchWorkLoad();
        newSrcAppModule->par("numPacketsToSend").setIntValue(newFlowSize); //
    }
    // <<<<<<<<

    //   --------<ndpIn         appOut[]<----------
    //     ndpApp                          ndp
    //   -------->ndpOut        appIn[] >----------
    cGate *gateRdpIn = rdpSrcModule->gate("in", newRDPGateInSizeSrc - 1);
    cGate *gateRdpOut = rdpSrcModule->gate("out", newRDPGateOutSizeSrc - 1);
    cGate *gateIn = newSrcAppModule->gate("socketIn");
    cGate *gateOut = newSrcAppModule->gate("socketOut");
    gateRdpOut->connectTo(gateIn);
    gateOut->connectTo(gateRdpIn);
    newSrcAppModule->finalizeParameters();
    newSrcAppModule->buildInside();
    newSrcAppModule->scheduleStart(simTime());
    //std::cout << "\nnewSrcAppModule callInitialize()" << std::endl;
    newSrcAppModule->callInitialize();
    newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);
}

int CentralSchedulerRdp::findNumSumbodules(cModule *nodeModule, const char *subModuleType)
{
    int rep = 0;
    for (cModule::SubmoduleIterator iter(nodeModule); !iter.end(); iter++) {
        cModule *subModule = *iter;
        if (strcmp(subModule->getModuleType()->getFullName(), subModuleType) == 0) {
            rep++;
        }
    }
    return rep;
}

void CentralSchedulerRdp::deleteAllSubModuleApp(const char *subModuleToBeRemoved) //due to established apps within INI
{
    std::cout << "\n\n ******************** deleteAll temp SubModuleApp  .. ********************  \n";
    std::string node;
    for (int i = 0; i < numServers; i++) {
        CentralSchedulerRdp::findLocation(i, node);
        cModule *nodeModule = getModuleByPath(node.c_str());
        //std::cout << "NODE: "<< node.c_str() << std::endl;
        // delete ndpApp[0]

        cModule *tempTcpAppModule = nullptr;
        for (cModule::SubmoduleIterator iter(nodeModule); !iter.end(); iter++) {
            cModule *subModule = *iter;
            if (strcmp(subModule->getFullName(), subModuleToBeRemoved) == 0) {

                tempTcpAppModule = subModule;
            }
        }
        tempTcpAppModule->deleteModule();
//        cModule* rdpSrcModule = nodeModule->getSubmodule("at");
//        for(int i = 0; i < rdpSrcModule->gateSize("out"); i++){
//            rdpSrcModule->gate("out", i)->disconnect();
//        }
//        for(int i = 0; i < rdpSrcModule->gateSize("in"); i++){
//                    rdpSrcModule->gate("in", i)->disconnect();
//        }
        //rdpSrcModule->gateSize("out")
        //rdpSrcModule->gate("out")->disconnect();
        //rdpSrcModule->gate("in")->disconnect();
        //rdpSrcModule->setGateSize("out", 0);
        //rdpSrcModule->setGateSize("in", 0);
    }
    std::cout << " Done.. \n";
}

void CentralSchedulerRdp::finish()
{
    myfile.close();
    for (std::vector<int>::iterator iter = permServers.begin(); iter != permServers.end(); ++iter) {
        cout << "  NODE= " << *iter << "  ";
        nodes.record(*iter);

        std::string source;
        CentralSchedulerRdp::findLocation(*iter, source); // get dest value
//        cout << "  nodePodRackLoc:  " << iter->second << " == " << source << " ==> " << iter->first << " == " << dest << "\n";
        cModule *srcModule = getModuleByPath(source.c_str());

        int finalNumTcpSessionApps = findNumSumbodules(srcModule, "ndp.application.ndpapp.RdpBasicClientApp");
        int finalNumTcpSinkApps = findNumSumbodules(srcModule, "rdp.application.rdpapp.RdpSinkApp");

        std::cout << "  finalNumTcpSessionApps:  " << finalNumTcpSessionApps << ",  finalNumTcpSinkApps: " << finalNumTcpSinkApps << "\n";
        numTcpSessionAppsVec.record(finalNumTcpSessionApps);
        numTcpSinkAppsVec.record(finalNumTcpSinkApps);
    }

    std::cout << "numshortflowRunningServers:  " << numshortflowRunningServers << std::endl;
    std::cout << "numlongflowsRunningServers:  " << numlongflowsRunningServers << std::endl;

    std::cout << "permLongFlowsServers:       ";
    for (std::vector<int>::iterator it = permLongFlowsServers.begin(); it != permLongFlowsServers.end(); ++it)
        std::cout << ' ' << *it;
    std::cout << '\n';

    // Record end time
    t2 = high_resolution_clock::now();
    auto duration = duration_cast<minutes>(t2 - t1).count();
    std::cout << "=================================================== " << std::endl;
    std::cout << " total Wall Clock Time (Real Time) = " << duration << " minutes" << std::endl;
    std::cout << " total Simulation Time      = " << totalSimTime << " sec" << std::endl;
    std::cout << "=================================================== " << std::endl;
    std::cout << " num completed shortflows = " << numCompletedShortFlows << std::endl;

    recordScalar("simTimeTotal=", totalSimTime);
    recordScalar("numShortFlows=", numShortFlows);
    recordScalar("flowSize=", flowSize);
    recordScalar("percentLongFlowNodes=", percentLongFlowNodes);
    recordScalar("arrivalRate=", arrivalRate);
    if (strcmp(trafficMatrixType, "permTM") == 0)
        recordScalar("randTM", 0);
    if (strcmp(trafficMatrixType, "randTM") == 0)
        recordScalar("randTM", 1);
    recordScalar("wallClockTime=", duration);

    recordScalar("IW=", IW);
    recordScalar("rdpSwitchQueueLength=", rdpSwitchQueueLength);

    recordScalar("perFlowEcmp=", perFlowEcmp);
    recordScalar("perPacketEcmp=", perPacketEcmp);
    recordScalar("oneToOne=", oneToOne);
    recordScalar("oneToMany=", oneToMany);
    recordScalar("manyToOne=", manyToOne);
    recordScalar("seedValue=", seedValue);
    recordScalar("kValue=", kValue);

    recordScalar("numReplica=", numReplica);
    recordScalar("numRunningMulticastGroups=", numRunningMulticastGroups);
    recordScalar("numRunningMultiSourcingGroups=", numRunningMultiSourcingGroups);
    recordScalar("isWebSearchWorkLoad=", isWebSearchWorkLoad);

    //   int i=0;
    std::list<RecordMat>::iterator itt;
    itt = recordMatList.begin();
    while (itt != recordMatList.end()) {
        matSrc.record(itt->recordSrc);
        matDest.record(itt->recordDest);
        //        std::cout << " flowNumber = " << ++i  << " src: " << itt->recordSrc << " , destttttt= " << itt->recordDest << std::endl;
        itt++;
    }
}

void CentralSchedulerRdp::handleParameterChange(const char *parname)
{
    std::cout << "\n CentralSchedulerRdp num completed shortflows = " << numCompletedShortFlows << std::endl;

    if (parname && strcmp(parname, "numCompletedShortFlows") == 0) {
//        numCompletedShortFlows = par("numCompletedShortFlows");
        ++numCompletedShortFlows;
        std::cout << " num completed shortflows = " << numCompletedShortFlows << "\n\n\n\n";

        if (oneToOne == true && numCompletedShortFlows == numShortFlows && oneToMany == false && manyToOne == false) {
//             getSimulation()->callFinish();
            scheduleAt(simTime(), stopSimulation);
        }

        if (oneToMany == true && manyToOne == false && oneToOne == false && numCompletedShortFlows == numReplica * numRunningMulticastGroups) {
            scheduleAt(simTime(), stopSimulation);
        }

        if (oneToMany == false && manyToOne == true && oneToOne == false && numCompletedShortFlows == numReplica * numRunningMultiSourcingGroups) {
            scheduleAt(simTime(), stopSimulation);
        }

    }
}

void CentralSchedulerRdp::getWebSearchWorkLoad()
{
    int numFlows = numShortFlows;
    double a[numFlows];
    std::ifstream myfile("../inputWokLoad.txt");
    if (myfile.is_open()) {
        int i = 0;
        while (i < numFlows && myfile >> a[i]) {
            flowSizeWebSeachWorkLoad.push_back(a[i]);
            std::cout << a[i] << " ";
            i++;
        }
        myfile.close();
    }
    else
        std::cerr << "Unable to open file" << endl;

    for (auto iter = flowSizeWebSeachWorkLoad.begin(); iter != flowSizeWebSeachWorkLoad.end(); ++iter) {
//           std::cout << " \nooooo = " << *iter << "\n";
    }
}

int CentralSchedulerRdp::getNewFlowSizeFromWebSearchWorkLoad()
{
    int newFLowSize = flowSizeWebSeachWorkLoad.at(indexWorkLoad);
    ++indexWorkLoad;
    return newFLowSize;
}

// 0     --> 10KB    P=1
// 10KB  --> 100KB   P=2
// 100KB --> 1MB     P=3
// 1MB   --> 10MB    P=4
// ;;;;;;;;;;;;;;;;;;

// random TM for multicasting (multicast 3 replicas)
void CentralSchedulerRdp::getNewThreeDestRandTMForMulticast(std::string &itsSrc, std::vector<std::string> &newDest)
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << "******************** getNewThreeDestRandTMForoneToMany    3 replicas multicast .. ********************  \n";
//    int newDestination = test;
    int newDestination = 0;
    int srcNewDestination = 0;
    int oldDest;

    std::vector<int> destinationNodes;
    std::vector<int> sortedNodes;

    srcNewDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
    newDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
    std::string selectedDest;
    for (int i = 0; i < numReplica; ++i) {
        while (newDestination == srcNewDestination) {
            newDestination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
            if (newDestination == oldDest)
                newDestination = srcNewDestination; // to get unique destinations for this multicast group
            //    int srcNewDestination = permMapShortFlows.find(newDestination)->second; // this line is used wit premTM not randTM
//            std::cout << "wwwwwwww " << newDestination << " , multicast source node   " << srcNewDestination << "\n";

        }
        std::cout << "@@@ newDestination " << newDestination << " , multicast source node   " << srcNewDestination << "\n";
        oldDest = newDestination;

        destinationNodes.push_back(newDestination);

        // identifying the servers locations: FatTreeTopology.Pod[].racks[].servers[]
//        CentralSchedulerRdp::findLocation(newDestination, selectedDest);
//        newDest.push_back(selectedDest);
        newDestination = srcNewDestination;
    }

    // NB: this only works for 3 replicas
    // 162 --> destinationNodes (13--> 90 --> 234)
    // 162 --> sortedNodes(90 --> 234 --> 13)
    // 162 --> sortedNodes4 (90 --> 13 --> 234)
    sortDaisyChainNodesBasedOnTopologicallyNearest(srcNewDestination, destinationNodes, sortedNodes);

    std::vector<int> sortedNodes2(2);
    sortedNodes2.at(0) = sortedNodes.at(1);
    sortedNodes2.at(1) = sortedNodes.at(2);
    std::vector<int> sortedNodes3;

    sortDaisyChainNodesBasedOnTopologicallyNearest(sortedNodes.at(0), sortedNodes2, sortedNodes3);

    std::vector<int> sortedNodes4(3);
    sortedNodes4.at(0) = sortedNodes.at(0);
    sortedNodes4.at(1) = sortedNodes3.at(0);
    sortedNodes4.at(2) = sortedNodes3.at(1);

    for (auto iter = sortedNodes4.begin(); iter != sortedNodes4.end(); ++iter) {
        std::cout << " moh ........ " << *iter << "\n\n\n";

        CentralSchedulerRdp::findLocation(*iter, selectedDest);
        newDest.push_back(selectedDest);
    }

    CentralSchedulerRdp::findLocation(srcNewDestination, itsSrc);
}

// daisy chain GFS sorting the selected destination nodes (src-->replica1-->replica2-->replica3) based on nearest
//  Sender writes to the topologically nearest replica
void CentralSchedulerRdp::sortDaisyChainNodesBasedOnTopologicallyNearest(int sourceNode, std::vector<int> destinationNodes, std::vector<int> &sortedNodes)
{
    std::vector<int> diff;
//    struct diffDest {
//        int diff, dest ;
//    };
    int difference;
    std::vector<differenceBetweenSrcNodeAndDestNode> diffDestValues;
    differenceBetweenSrcNodeAndDestNode diffDestStru;
    for (auto iter = destinationNodes.begin(); iter != destinationNodes.end(); ++iter) {
        if (sourceNode >= *iter) {
            difference = sourceNode - *iter;
            diffDestStru.dest = *iter;
            diffDestStru.diff = difference;
            diffDestValues.push_back(diffDestStru);
        }
        else {
            difference = *iter - sourceNode;
            diffDestStru.dest = *iter;
            diffDestStru.diff = difference;
            diffDestValues.push_back(diffDestStru);
        }
    }

//    for (auto x : diffDestValues)
//            cout << "[" << x.diff << ", " << x.dest << "] ";

    std::sort(diffDestValues.begin(), diffDestValues.end());
    for (auto iter = diffDestValues.begin(); iter != diffDestValues.end(); ++iter) {
        sortedNodes.push_back(iter->dest);
//        cout << " diff = " << iter->diff << " dest" << iter->dest << "\n";
    }

}

// random TM for manyToOne (fetch 3 replicas) multisourcing
void CentralSchedulerRdp::getNewThreeSrcRandTMForMultiSourcing(std::string &destNode, std::vector<std::string> &senders)
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << "******************** getNew  Three  Src RandTM  For   MultiSourcing   fetch 3 replicas   .. ********************  \n";
    //    int newDestination = test;
    int src = 0;
    int destination = 0;
    int oldDest;

    destination = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
    src = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
    std::string selectedSrc;
    for (int i = 0; i < numReplica; ++i) {
        while (src == destination) {
            src = permServers.at(numlongflowsRunningServers + (std::rand() % (numshortflowRunningServers)));
            if (src == oldDest)
                src = destination; // to get unique destinations for this multicast group
            //    int srcNewDestination = permMapShortFlows.find(newDestination)->second; // this line is used wit premTM not randTM
            //            std::cout << "wwwwwwww " << newDestination << " , multicast source node   " << srcNewDestination << "\n";

        }
        std::cout << "@@@ MultiSourcing src " << src << " ,  dest   " << destination << "\n";
        oldDest = src;

        // identifying the servers locations: FatTreeTopology.Pod[].racks[].servers[]
        CentralSchedulerRdp::findLocation(src, selectedSrc);
        senders.push_back(selectedSrc);
        src = destination;
    }
    CentralSchedulerRdp::findLocation(destination, destNode);
}

void CentralSchedulerRdp::scheduleNewDaisyChainSession(std::string itsSrc, std::vector<std::string> newDest, int multicastGrpId) // three replicas
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@ scheduleNewMultiCastSession three replicas .. @@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << " itsSrc " << itsSrc << "\n";

    newArrivalTime = expDistribution.operator()(PRNG);
    sumArrivalTimes = sumArrivalTimes + newArrivalTime;

    // multicasting
    // ===================================== Daisy Chain GFS workload ============================================
    //            Sender                                                                           ===
    //       --------------------------------                           -------------------        ===
    //       |  RdpBasicClientApp --> conn1 -|-------------------->     | Receiver 1 Sink  |       ===
    //       --------------------------------                            ------------------        ===
    //                                                                           v
    //                                                                           v
    //                                                                   ------------------        ===
    //                                                                  | Receiver 2 Sink  |       ===
    //                                                                   ------------------        ===
    //                                                                           v
    //                                                                           v
    //                                                                   ------------------        ===
    //                                                                  | Receiver 3 Sink  |       ===
    //                                                                   ------------------        ===

    // NOTE: all connections start at the same sumArrivalTimes (this is not realistic)
    // In reality: Pipelined: each replica forwards as it receives
    //      goodput = ave(goodputs: sink1,sink2,sink3)                                            ===
    // ==============================================================================================

//    std::cout << "srcModule:  " << srcModule->getFullPath() << "  , destModule:  " << destModule->getFullPath() << std::endl << std::endl;
    for (auto iter = newDest.begin(); iter != newDest.end(); ++iter) {
        std::string thisDest = *iter;
        std::cout << " \n\n\n\n NEW conn in DAISY CHAIN ........... " << newArrivalTime << "\n";

        std::cout << " itsSrc " << itsSrc << "\n";
        std::cout << " newDest " << thisDest << "\n";
        cModule *destModule = getModuleByPath(thisDest.c_str());
        cModule *rdpDestModule = destModule->getSubmodule("at");
        int newRdpGateOutSizeDest = rdpDestModule->gateSize("out") + 1;
        int newRdpGateInSizeDest = rdpDestModule->gateSize("in") + 1;
        rdpDestModule->setGateSize("out", newRdpGateOutSizeDest);
        rdpDestModule->setGateSize("in", newRdpGateInSizeDest);
        int newNumRdpSinkAppsDest = findNumSumbodules(destModule, "rdp.application.rdpapp.RdpSinkApp") + 1;
        std::cout << "Dest  NumTCPSinkApp   =  " << newNumRdpSinkAppsDest << "\n";
        cModuleType *moduleTypeDest = cModuleType::get("rdp.application.rdpapp.RdpSinkApp"); // find factory object
        std::string nameRdpAppDest = "app[" + std::to_string(newNumRdpSinkAppsDest - 1) + "]";

        std::string sinkName = thisDest + "." + nameRdpAppDest;

        myfile << sinkName << "\n";

        std::cout << " newDest sinkName: " << sinkName << "\n";

        cModule *newDestAppModule = moduleTypeDest->create(nameRdpAppDest.c_str(), destModule);
        newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);

        //newDestAppModule->par("multiCastGroupId").setDoubleValue(
        //        multicastGrpId);

        cGate *gateRdpInDest = rdpDestModule->gate("in", newRdpGateOutSizeDest - 1);
        cGate *gateRdpOutDest = rdpDestModule->gate("out", newRdpGateOutSizeDest - 1);
        cGate *gateInDest = newDestAppModule->gate("socketIn");
        cGate *gateOutDest = newDestAppModule->gate("socketOut");
        gateRdpOutDest->connectTo(gateInDest);
        gateOutDest->connectTo(gateRdpInDest);
        newDestAppModule->finalizeParameters();
        newDestAppModule->buildInside();
        newDestAppModule->scheduleStart(simTime());
        newDestAppModule->callInitialize();

        // get the src ndp module
        cModule *srcModule = getModuleByPath(itsSrc.c_str()); // const char* c_str Return pointer to the string.
        cModule *rdpSrcModule = srcModule->getSubmodule("at");
        int newRDPGateOutSizeSrc = rdpSrcModule->gateSize("out") + 1;
        int newRDPGateInSizeSrc = rdpSrcModule->gateSize("in") + 1;
        rdpSrcModule->setGateSize("out", newRDPGateOutSizeSrc);
        rdpSrcModule->setGateSize("in", newRDPGateInSizeSrc);
        int newNumRdpSessionAppsSrc = findNumSumbodules(srcModule, "ndp.application.ndpapp.RdpBasicClientApp") + 1;
        std::cout << "Src  numTCPSessionApp =  " << newNumRdpSessionAppsSrc << "\n";
        cModuleType *moduleTypeSrc = cModuleType::get("ndp.application.ndpapp.RdpBasicClientApp"); // find factory object
        std::string nameRdpAppSrc = "app[" + std::to_string(newNumRdpSessionAppsSrc - 1) + "]";
        cModule *newSrcAppModule = moduleTypeSrc->create(nameRdpAppSrc.c_str(), srcModule);

        newSrcAppModule->par("connectAddress").setStringValue(thisDest);
        newSrcAppModule->par("connectPort").setIntValue(80 + newNumRdpSinkAppsDest);  //??? to be checked

        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);

//        newSrcAppModule->par("numPacketsToSend").setDoubleValue(flowSize); //
        /// >>>>>>>>>>>>
        if (isWebSearchWorkLoad == false) {
            newSrcAppModule->par("numPacketsToSend").setDoubleValue(flowSize); //
            // aha .....
        }

        if (isWebSearchWorkLoad == true) {
            int newFlowSize = getNewFlowSizeFromWebSearchWorkLoad();
            newSrcAppModule->par("numPacketsToSend").setIntValue(newFlowSize); //
        }
        /// >>>>>>>>>>>

        cGate *gateRdpIn = rdpSrcModule->gate("in", newRDPGateInSizeSrc - 1);
        cGate *gateRdpOut = rdpSrcModule->gate("out", newRDPGateOutSizeSrc - 1);
        cGate *gateIn = newSrcAppModule->gate("socketIn");
        cGate *gateOut = newSrcAppModule->gate("socketOut");
        gateRdpOut->connectTo(gateIn);
        gateOut->connectTo(gateRdpIn);
        newSrcAppModule->finalizeParameters();
        newSrcAppModule->buildInside();
        newSrcAppModule->scheduleStart(simTime());
        newSrcAppModule->callInitialize();
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);
        //
        // NEW daisy chain next
        //
        itsSrc = thisDest;
//         newArrivalTime=newArrivalTime+0.000001;
    }
}

void CentralSchedulerRdp::scheduleNewMultiCastSession(std::string itsSrc, std::vector<std::string> newDest, int multicastGrpId) // three replicas
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@ scheduleNewMultiCastSession three replicas .. @@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << " itsSrc " << itsSrc << "\n";

    newArrivalTime = expDistribution.operator()(PRNG);
    sumArrivalTimes = sumArrivalTimes + newArrivalTime;

    // multicasting
    // ===================================== ONE SESSION ============================================
    //            Sender                                                                          ===
    //       --------------------------------                           -------------------       ===
    //       |  RdpBasicClientApp --> conn1 -|-------------------->    | Receiver 1 Sink  |       ===
    //       |                               |                           ------------------       ===
    //       |                               |                           ------------------       ===
    //       |  RdpBasicClientApp --> conn2 -|-------------------->    | Receiver 2 Sink  |       ===
    //       |                               |                           ------------------       ===
    //       |                               |                           ------------------       ===
    //       |  RdpBasicClientApp --> conn3 -|-------------------->    | Receiver 3 Sink  |       ===
    //       --------------------------------                           -------------------       ===
    //      goodput = ave(goodputs: sink1,sink2,sink3)                                            ===
    // ==============================================================================================

//    std::cout << "srcModule:  " << srcModule->getFullPath() << "  , destModule:  " << destModule->getFullPath() << std::endl << std::endl;
    for (auto iter = newDest.begin(); iter != newDest.end(); ++iter) {
        std::string thisDest = *iter;
        std::cout << " newDest " << thisDest << "\n";
        cModule *destModule = getModuleByPath(thisDest.c_str());
        cModule *rdpDestModule = destModule->getSubmodule("at");
        int newRdpGateOutSizeDest = rdpDestModule->gateSize("out") + 1;
        int newRdpGateInSizeDest = rdpDestModule->gateSize("in") + 1;
        rdpDestModule->setGateSize("out", newRdpGateOutSizeDest);
        rdpDestModule->setGateSize("in", newRdpGateInSizeDest);
        int newNumRdpSinkAppsDest = findNumSumbodules(destModule, "rdp.application.rdpapp.RdpSinkApp") + 1;
        std::cout << "Dest  NumTCPSinkApp   =  " << newNumRdpSinkAppsDest << "\n";
        cModuleType *moduleTypeDest = cModuleType::get("rdp.application.rdpapp.RdpSinkApp"); // find factory object
        std::string nameRdpAppDest = "app[" + std::to_string(newNumRdpSinkAppsDest - 1) + "]";

        std::string sinkName = thisDest + "." + nameRdpAppDest;

        myfile << sinkName << "\n";

        std::cout << " newDest sinkName: " << sinkName << "\n";

        cModule *newDestAppModule = moduleTypeDest->create(nameRdpAppDest.c_str(), destModule);
        newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);

        //newDestAppModule->par("multiCastGroupId").setDoubleValue(
        //        multicastGrpId);

        cGate *gateRdpInDest = rdpDestModule->gate("in");
        cGate *gateRdpOutDest = rdpDestModule->gate("out");
        cGate *gateInDest = newDestAppModule->gate("socketIn");
        cGate *gateOutDest = newDestAppModule->gate("socketOut");
        gateRdpOutDest->connectTo(gateInDest);
        gateOutDest->connectTo(gateRdpInDest);
        newDestAppModule->finalizeParameters();
        newDestAppModule->buildInside();
        newDestAppModule->scheduleStart(simTime());
        newDestAppModule->callInitialize();

        // get the src ndp module
        cModule *srcModule = getModuleByPath(itsSrc.c_str()); // const char* c_str Return pointer to the string.
        cModule *rdpSrcModule = srcModule->getSubmodule("at");
        int newRDPGateOutSizeSrc = rdpSrcModule->gateSize("out") + 1;
        int newRDPGateInSizeSrc = rdpSrcModule->gateSize("in") + 1;
        rdpSrcModule->setGateSize("out", newRDPGateOutSizeSrc);
        rdpSrcModule->setGateSize("in", newRDPGateInSizeSrc);
        int newNumRdpSessionAppsSrc = findNumSumbodules(srcModule, "ndp.application.ndpapp.RdpBasicClientApp") + 1;
        std::cout << "Src  numTCPSessionApp =  " << newNumRdpSessionAppsSrc << "\n";
        cModuleType *moduleTypeSrc = cModuleType::get("ndp.application.ndpapp.RdpBasicClientApp"); // find factory object
        std::string nameRdpAppSrc = "app[" + std::to_string(newNumRdpSessionAppsSrc - 1) + "]";
        cModule *newSrcAppModule = moduleTypeSrc->create(nameRdpAppSrc.c_str(), srcModule);

        newSrcAppModule->par("connectAddress").setStringValue(thisDest);
        newSrcAppModule->par("connectPort").setIntValue(80 + newNumRdpSinkAppsDest);  //??? to be checked

        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);

//        newSrcAppModule->par("numPacketsToSend").setDoubleValue(flowSize); //
        /// >>>>>>>>>>>>
        if (isWebSearchWorkLoad == false) {
            newSrcAppModule->par("numPacketsToSend").setIntValue(flowSize); //
        }

        if (isWebSearchWorkLoad == true) {
            int newFlowSize = getNewFlowSizeFromWebSearchWorkLoad();
            newSrcAppModule->par("numPacketsToSend").setIntValue(newFlowSize); //
        }
        /// >>>>>>>>>>>

        cGate *gateRdpIn = rdpSrcModule->gate("in");
        cGate *gateRdpOut = rdpSrcModule->gate("out");
        cGate *gateIn = newSrcAppModule->gate("socketIn");
        cGate *gateOut = newSrcAppModule->gate("socketOut");
        gateRdpOut->connectTo(gateIn);
        gateOut->connectTo(gateRdpIn);
        newSrcAppModule->finalizeParameters();
        newSrcAppModule->buildInside();
        newSrcAppModule->scheduleStart(simTime());
        newSrcAppModule->callInitialize();
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);
    }
}

void CentralSchedulerRdp::scheduleNewMultiSourcingSession(std::string dest, std::vector<std::string> senders, int multiSrcGroupId) // three replicas
{
    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@ scheduleNewMultiSourcingSession fetch three replicas  multi sourcing .. @@@@@@@@@@@@@@@@@@@@@@@@@  \n";
    std::cout << " dest nodes: " << dest << "\n";

// this block used to be here not in the for loop below
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>>
    //    newArrivalTime = expDistribution.operator()(PRNG);   // >>>>>>>>>
    //    sumArrivalTimes = sumArrivalTimes + newArrivalTime;  // >>>>>>>>
    // >>>>>>>>>>>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>>

//    std::cout << "srcModule:  " << srcModule->getFullPath() << "  , destModule:  " << destModule->getFullPath() << std::endl << std::endl;

// multi-sourcing
// ===================================== ONE SESSION ============================================
//          Receiver
//   --------        ------------
//   |      |        |   conn1  |   <------------------    Sender 1 (RdpBasicClientApp) object1
//   | SINK |  <---- |   conn2  |   <------------------    Sender 2 (RdpBasicClientApp) object2
//   |      |        |   conn3  |   <------------------    Sender 3 (RdpBasicClientApp) object3
//   --------        ------------
//      goodput = SINK ? but this gives high goodput as three connections serve the sink
// ==============================================================================================
// to use this way take  WAY-I block outside the for loop
    newArrivalTime = expDistribution.operator()(PRNG);
    sumArrivalTimes = sumArrivalTimes + newArrivalTime;
    for (auto iter = senders.begin(); iter != senders.end(); ++iter) {
//        ++test;
        // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>>
//            newArrivalTime = expDistribution.operator()(PRNG);   // >>>>>>>>>

//            sumArrivalTimes = sumArrivalTimes + newArrivalTime;
//
//             if (test < 3)  sumArrivalTimes = sumArrivalTimes + newArrivalTime;  // >>>>>>>>
//            if (test == 2)  sumArrivalTimes = sumArrivalTimes + newArrivalTime;  // >>>>>>>>

        // >>>>>>>>>>>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>> >>>>>>>>>>>

        // multi-sourcing
        //            Receiver
        //       --------------------
        //       |  sink1 <-- conn1  |   <------------------    Sender 1 (RdpBasicClientApp)
        //       |  sink2 <-- conn2  |   <------------------    Sender 2 (RdpBasicClientApp)
        //       |  sink3 <-- conn3  |   <------------------    Sender 3 (RdpBasicClientApp)
        //       ---------------------
        //      goodput = ave(goodputs: sink1,sink2,sink3)

        ////////////////////////////  WAY-I  /////////////////////////////////////
        std::cout << " dest: " << dest << "\n";
        cModule *destModule = getModuleByPath(dest.c_str());
        cModule *rdpDestModule = destModule->getSubmodule("at");
        int newRdpGateOutSizeDest = rdpDestModule->gateSize("out") + 1;
        int newRdpGateInSizeDest = rdpDestModule->gateSize("in") + 1;
        rdpDestModule->setGateSize("out", newRdpGateOutSizeDest);
        rdpDestModule->setGateSize("in", newRdpGateInSizeDest);
        int newNumRdpSinkAppsDest = findNumSumbodules(destModule, "rdp.application.rdpapp.RdpSinkApp") + 1;
        std::cout << "Dest  NumNDPSinkApp   =  " << newNumRdpSinkAppsDest << "\n";
        cModuleType *moduleTypeDest = cModuleType::get("rdp.application.rdpapp.RdpSinkApp"); // find factory object
        std::string nameRdpAppDest = "app[" + std::to_string(newNumRdpSinkAppsDest - 1) + "]";
        cModule *newDestAppModule = moduleTypeDest->create(nameRdpAppDest.c_str(), destModule);
        newDestAppModule->par("localPort").setIntValue(80 + newNumRdpSinkAppsDest);

        newDestAppModule->par("multiSrcGroupId").setDoubleValue(multiSrcGroupId); // added new

        cGate *gateRdpInDest = rdpDestModule->gate("in", newRdpGateInSizeDest - 1);
        cGate *gateRdpOutDest = rdpDestModule->gate("out", newRdpGateOutSizeDest - 1);
        cGate *gateInDest = newDestAppModule->gate("socketIn");
        cGate *gateOutDest = newDestAppModule->gate("socketOut");

        gateRdpOutDest->connectTo(gateInDest);
        gateOutDest->connectTo(gateRdpInDest);
        newDestAppModule->finalizeParameters();
        newDestAppModule->buildInside();
        newDestAppModule->scheduleStart(simTime());
        newDestAppModule->callInitialize();
        ////////////////////////////  WAY-I  /////////////////////////////////////

        // get the src ndp module
        std::string itsSender = *iter;
        cModule *srcModule = getModuleByPath(itsSender.c_str()); // const char* c_str Return pointer to the string.
        cModule *rdpSrcModule = srcModule->getSubmodule("at");
        int newRDPGateOutSizeSrc = rdpSrcModule->gateSize("out") + 1;
        int newRDPGateInSizeSrc = rdpSrcModule->gateSize("in") + 1;
        rdpSrcModule->setGateSize("out", newRDPGateOutSizeSrc);
        rdpSrcModule->setGateSize("in", newRDPGateInSizeSrc);
        int newNumRdpSessionAppsSrc = findNumSumbodules(srcModule, "rdp.application.rdpapp.RdpBasicClientApp") + 1;
        std::cout << "Src  numTCPSessionApp =  " << newNumRdpSessionAppsSrc << "\n";
        cModuleType *moduleTypeSrc = cModuleType::get("rdp.application.rdpapp.RdpBasicClientApp"); // find factory object
        std::string nameRdpAppSrc = "app[" + std::to_string(newNumRdpSessionAppsSrc - 1) + "]";
        cModule *newSrcAppModule = moduleTypeSrc->create(nameRdpAppSrc.c_str(), srcModule);

        newSrcAppModule->par("connectAddress").setStringValue(dest);
        newSrcAppModule->par("connectPort").setIntValue(80 + newNumRdpSinkAppsDest);  //??? to be checked

        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);

        /// >>>>>>>>>>>>
        if (isWebSearchWorkLoad == false) {
//               flowSize = flowSize/3; ??????????????????
            newSrcAppModule->par("numPacketsToSend").setIntValue(flowSize); //
            // aha .....
        }

        if (isWebSearchWorkLoad == true) {
            int newFlowSize = getNewFlowSizeFromWebSearchWorkLoad();
            newSrcAppModule->par("numPacketsToSend").setIntValue(newFlowSize); //
        }
        /// >>>>>>>>>>>>
//        newSrcAppModule->par("numPacketsToSend").setDoubleValue(flowSize); //

        cGate *gateRdpIn = rdpSrcModule->gate("in", newRDPGateInSizeSrc - 1);
        cGate *gateRdpOut = rdpSrcModule->gate("out", newRDPGateOutSizeSrc - 1);
        cGate *gateIn = newSrcAppModule->gate("socketIn");
        cGate *gateOut = newSrcAppModule->gate("socketOut");
        gateRdpOut->connectTo(gateIn);
        gateOut->connectTo(gateRdpIn);
        newSrcAppModule->finalizeParameters();
        newSrcAppModule->buildInside();
        newSrcAppModule->scheduleStart(simTime());
        newSrcAppModule->callInitialize();
        newSrcAppModule->par("startTime").setDoubleValue(simTime().dbl() + sumArrivalTimes);

        newArrivalTime = expDistribution.operator()(PRNG);
        sumArrivalTimes = sumArrivalTimes + newArrivalTime;
    }
}
}
