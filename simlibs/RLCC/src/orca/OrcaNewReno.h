
#ifdef ORCA
#ifndef _RL_MYTCPALGORITHM_H_
#define _RL_MYTCPALGORITHM_H_

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/tcp/flavours/TcpNewReno.h>

#include <RLInterface.h>
#include <PacedTcpConnection.h>


#define RTT_MULTIPLIER 1.7 // multiplier for the srtt when setting MI length
#define MI_MINIMUM_LENGTH 0.005 // default length of MI

using namespace inet::tcp;
using namespace inet;

typedef std::vector< std::tuple<int,string> > tuple_list;


namespace learning {

class OrcaNewReno : public TcpNewReno, public RLInterface
{
protected:


    //-------------------------------------------------------- LEGACY CODE ----------------------------------------------------------------------------------------------------------------------------------------------------

    // ssequence number ending with at a MI
    uint32 bytesAcked; //used for throughput

    uint32 previousUnacked; //used for throughput

    simtime_t previousUnackedTime; //used for throughput

    simtime_t currentUnackedTime; //used for throughput
    
    double throughputValue; //used for throughput

    double maxThroughputValue; //used for throughput
    
    double minAverageDelay = 1000000000000; //arbitary large number to be updated with the average delay
    
    double segmentSentAtMIStart = 0; //The average loss rate of packets per MI is the number of lost packets \ number of segments sent

    //-------------------------------------------------------- LEGACY CODE ----------------------------------------------------------------------------------------------------------------------------------------------------

    // ---- Track if RLInterface has been initialised
    bool rlInitialised = false;

    std::map<double, int> actionDistribution;

    double start_snd_una;
    double start_time;

    // ----- Per Step Variables 
    
    double throughput;
    double firstPacketSentSeqNum;
    simtime_t timeFirstPacketSent;
    std::map<double, std::vector<double>> times; // key = seq num value = {time packet sent, time packet recieved back at sender}
    double seqPacketLoss;


    double averageLossRateOfPackets;
    double numOfLostPackets; // the number of ack packets revieved in a MI excluding dup ack packets. The average loss rate of packets per MI is the number of lost packets \ number of segments sent
    double numOfPacketsSent;

    double averageDelayOfPackets; // the average delay of packets
    std::vector<double> RTTMI; // storing the RTT's for a MI

    double numOfValidAckPackets; // the number of ack packets revieved in a MI excluding dup ack packets.
    
    double timeBetweenLastReportAndCurrentReport; // time for the current MTP.
    double timeForNextMTP;


    // simtime_t previousStepTimestamp;
    // uint32_t numberOfDataPacketsStep;
    // uint32_t numberOfTrimmedPacketsStep;
    // uint32_t numberOfRecoveredDataPacketsStep;
    // uint32_t numberOfTrimmedBytesStep;
    // simtime_t sRttStep = SIMTIME_ZERO;
    // simtime_t minRttStep = SIMTIME_ZERO;
    // simtime_t rttvarStep = SIMTIME_ZERO;
    // simtime_t physLatency = SIMTIME_ZERO; // Latency of trimmed headers. Used aas en estimate of the physical latency between src and dest, since they should not incur into any queueing delay
    //double goodputStep; //Updated everytime we receive a new data packet.

    // ----- Per flow variable

    double smoothRTTOfPacketsSoFar;
    double currentCongestionWindow;
    double maxValueOfThroughputSoFar;
    double minValueOfPacketDelaySoFar;

    simtime_t lastReportTime;
    

    bool recievedAckslastMTP;
    
    // simtime_t sRtt = SIMTIME_ZERO;
    // simtime_t minRtt = SIMTIME_ZERO;
    // double goodput;
    // simtime_t latestRtt = SIMTIME_ZERO;
    // simtime_t rttvar = SIMTIME_ZERO;

    // Sorted STL to keep track of nacked packets. When a packet is nacked, it is inserted in the list.
    // Every time a packet is received, we check if it is a retransmission packet or not.
    // Useful in the case we want to compute RTT only on data that is not retransmitted
    //std::set<unsigned int> currentlyNackedPackets;
public:

    OrcaNewReno();

    // overridden methods from TcpNewReno and base classes
    virtual void initialize() override;

    virtual void established(bool active) override; //TODO: needed?

    virtual bool sendData(bool sendCommandInvoked) override;

    virtual void receivedDataAck(uint32 firstSeqAcked) override;
    virtual void receivedDuplicateAck() override;

    //virtual void processTimer(cMessage *timer, TcpEventCode &event) override;
    virtual void processRexmitTimer(TcpEventCode &event) override;

    // //We override this function because we want to store every single RTT computed.
    virtual void rttMeasurementComplete(simtime_t tSent, simtime_t tAcked) override;


    // implemented methods from RLInterface

    virtual void decisionMade(float action) override; // Call back from RLInterface. Called when the action from the agent has been received.

    virtual void cleanup() override; //TODO:is this function needed?
    
    //-------------------------------------------------------- LEGACY CODE ----------------------------------------------------------------------------------------------------------------------------------------------------
    virtual ObsType getRLState() override;
    virtual RewardType getReward() override;
    virtual bool getDone() override;
    //-------------------------------------------------------- LEGACY CODE ----------------------------------------------------------------------------------------------------------------------------------------------------
    virtual void resetStepVariables() override;
    virtual ObsType computeObservation() override;
    virtual RewardType computeReward() override;

    //other methods

    void initRLAgent();
    double computeAverageDelayOfPackets();
    double computeAverageLossRateOfPackets();
    double computeTimeBetweenLastReportAndCurrentReport();
    void computeTimeForNextMTP();

    double computeCurrentCongestionWindow();
    double computeMinValueOfPacketDelaySoFar();
    double computeThroughput();
    double computeMaxValueOfThroughputSoFar();
    double computeInitialAverageDelayOfPackets(); //MTP length for reset 



};
}
#endif /* TRANSPORTLAYER_RL_RLTCPNEWRENO_H_ */
#endif