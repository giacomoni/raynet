import transportlayer.rdp.RdpConnectionState;

cplusplus {{
typedef std::set<unsigned int> AckStore;
}}

class AckStore
{
   @existingClass;
   //FIXME tostring, ...
};

namespace inet::rdp;

struct RLRdpAlgStateVariables extends RdpStateVariables
{

    @descriptor(readonly); 
    // ----- Per Step Variables 
    simtime_t previousStepTimestamp = 0;
    uint32_t numberOfDataPacketsStep = 0;
    uint32_t numberOfTrimmedPacketsStep = 0;
    uint32_t numberOfRecoveredDataPacketsStep = 0;
    uint32_t numberOfTrimmedBytesStep = 0;
    double goodputStep = 0; //Updated everytime we receive a new data packet.

    // ----- Per flow variable
    double goodput = 0;
    double estLinkRate = 0;

    // Sorted STL to keep track of nacked packets. When a packet is nacked, it is inserted in the list.
    // Every time a packet is received, we check if it is a retransmission packet or not.
    // Useful in the case we want to compute RTT only on data that is not retransmitted
    AckStore currentlyNackedPackets;

    //Weights for the reward funtion
    double thrWeight;
    double trimWeight;
    double delayWeight;

    int consecutiveBadSteps = 0;

    double timeCwndChanged = 0;
    int cwndReport = 0;

    //Slow Start
    bool slowStart = true;
    uint32_t packetsInFlight = 0;

};




