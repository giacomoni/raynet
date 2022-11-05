#ifndef __INET_CentralSchedulerRdp_H
#define __INET_CentralSchedulerRdp_H
#include <chrono>  // for high_resolution_clock
#include <fstream>
#include <map>
#include <set>
#include <algorithm>    // std::random_shuffle
#include <vector>
#include <random>
#include <cmath>
#include <time.h>
#include <fstream>

#include <inet/common/INETDefs.h>
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/common/lifecycle/ILifecycle.h>

#include "../../application/rdpapp/RdpBasicClientApp.h"
#include "../../application/rdpapp/RdpSinkApp.h"
#include <algorithm>

namespace inet{

class INET_API CentralSchedulerRdp: public cSimpleModule, public ILifecycle {

protected:
    bool isWebSearchWorkLoad;
    int indexWorkLoad;
    std::vector<int> flowSizeWebSeachWorkLoad;

    std::ofstream myfile;

    // daisyChainGFS: sorting the nodes (1 source& 3 replicas) based on how far is each dest from the source
    struct differenceBetweenSrcNodeAndDestNode {
        int diff, dest ;
        bool operator<(const differenceBetweenSrcNodeAndDestNode& a) const
          {
              return diff < a.diff; // ascending order
          }
    };

    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;
    simtime_t totalSimTime;
    cOutVector permMapLongFlowsVec;
    cOutVector permMapShortFlowsVec;

    cOutVector randMapShortFlowsVec;
    cOutVector permMapShortFlowsVector;

    cOutVector numTcpSessionAppsVec;
    cOutVector numTcpSinkAppsVec;
    cOutVector nodes;
    cOutVector matSrc; // record all the source servers of the created short flows
    cOutVector matDest; // record all the dest servers of the created short flows

    cMessage *startManagerNode;
    int kValue;

    int IW;
    int rdpSwitchQueueLength;
    bool perFlowEcmp;
    bool perPacketEcmp;

    const char *trafficMatrixType; // either "permTM"  or "randTM"
    int test = 0;
    int arrivalRate; // lamda of an exponential distribution (Morteza uses 256 and 2560)
    int flowSize;
    int numServers;
    int numShortFlows;
    int longFlowSize;
    double percentLongFlowNodes;
    int numCompletedShortFlows = 0;
    cMessage *stopSimulation;
    std::vector<int> permServers;

    std::vector<int> permLongFlowsServers;
    std::vector<int> permShortFlowsServers;

    int numlongflowsRunningServers; // 33% of nodes run long flows
    int numshortflowRunningServers;

    int numIncastSenders;
    ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
    bool oneToOne = false;
//    const char *runMulticast; // for   multicasting
    bool oneToMany; // for   multicasting
    int numRunningMulticastGroups;
//    const char * manyToOne;
    bool manyToOne;
    int  numRunningMultiSourcingGroups;
    int  numReplica;
    bool daisyChainGFS;
////////////////////////////////////////////////////////////////
 ////////////////////////////////////////////////////////////////

    virtual bool handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback) override
            {
        Enter_Method_Silent(); throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName()); return true;}

    //  <dest, src>
    std::map<int, int> permMapLongFlows;
    std::map<int, int> permMapShortFlows;

    double sumArrivalTimes = 0;
    double newArrivalTime;
    bool shuffle = false;

    struct NodeLocation
    {
        int pod;
        int rack;
        int node;
        int index;

        int numTCPSink;
        int numTCPSession;
    };

    typedef std::list<NodeLocation> NodeLocationList;
    NodeLocationList nodeLocationList;

    int seedValue;
    std::mt19937 PRNG;
    std::exponential_distribution<double> expDistribution;

    struct RecordMat
    {
        int recordSrc;
        int recordDest;
    };
    typedef std::list<RecordMat> RecordMatList;
    RecordMatList recordMatList;


public:
    CentralSchedulerRdp()
    {
    }
    virtual ~CentralSchedulerRdp();

protected:
    virtual void initialize(int stage) override;
    //virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleParameterChange(const char *parname) override;

    void serversLocations();
    void generateTM();

    void getNewDestRandTM(std::string& itsSrc, std::string& newDest);
    void getNewDestPremTM(std::string& itsSrc, std::string& newDest);

    void findLocation(int nodeIndex, std::string& nodePodRackLoc);
    void scheduleLongFlows();
    void deleteAllSubModuleApp(const char *subModuleToBeRemoved);
    int findNumSumbodules(cModule* nodeModule, const char *subModuleType);
    void scheduleNewShortFlow(std::string itsSrc, std::string newDest);


    // multicast & multiSourcing
    void getNewThreeDestRandTMForMulticast(std::string& itsSrc, std::vector<std::string>& newDest );
    void scheduleNewMultiCastSession(std::string itsSrc, std::vector<std::string> newDest , int multicastGrpId);
    void scheduleNewDaisyChainSession(std::string itsSrc, std::vector<std::string> newDest , int multicastGrpId);

    void sortDaisyChainNodesBasedOnTopologicallyNearest(int sourceNode, std::vector<int> destinationNodes,  std::vector<int>& sortedNodes);
    void getNewThreeSrcRandTMForMultiSourcing(std::string& destNode, std::vector<std::string>& senders );
    void scheduleNewMultiSourcingSession(std::string dest, std::vector<std::string> senders , int multiSrcGroupId);

    void scheduleIncast(int numSenders);
    void getWebSearchWorkLoad();
    int getNewFlowSizeFromWebSearchWorkLoad();

};

}

#endif // ifndef __INET_RQ_H

