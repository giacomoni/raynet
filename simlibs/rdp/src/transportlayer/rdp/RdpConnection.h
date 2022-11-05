#ifndef __INET_RdpConnection_H
#define __INET_RdpConnection_H

#include <inet/common/INETDefs.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/common/packet/ChunkQueue.h>
#include <queue>
#include <map>

#include "../../transportlayer/rdp/Rdp.h"
#include "../rdp/rdp_common/RdpHeader.h"

namespace inet {

class RdpCommand;
class RdpOpenCommand;

namespace rdp {

class RdpHeader;
class RdpSendQueue;
class RdpReceiveQueue;
class RdpAlgorithm;

enum RdpState
{
    RDP_S_INIT = 0, RDP_S_CLOSED = FSM_Steady(1), RDP_S_LISTEN = FSM_Steady(2), RDP_S_SYN_SENT = FSM_Steady(3), RDP_S_SYN_RCVD = FSM_Steady(4), RDP_S_ESTABLISHED = FSM_Steady(5),
};

//
// Event, strictly for the FSM state transition purposes.
// DO NOT USE outside performStateTransition()!
//
enum RdpEventCode
{
    RDP_E_IGNORE,

    // app commands
    RDP_E_OPEN_ACTIVE,
    RDP_E_OPEN_PASSIVE,
    RDP_E_RCV_DATA,
    RDP_E_RCV_SYN,
};


// Helper class storing a single measurement value and its timestmp
class Measurement{
    private:
        double measurement;
        simtime_t timestamp;
    public: 
        Measurement(double _measurement, simtime_t _timestamp){measurement=_measurement; timestamp=_timestamp;};
        double getMeasurement(){return measurement;};
        simtime_t getTimestamp(){return timestamp;};
};

class Estimator{
    private:
        double windowSize; // Size of the window in time (s)
        std::vector<Measurement> samples;
        int flushCounter;

    public:
        Estimator();
        void setWindowSize(double _windowSize);
        void addSample(double measurement, simtime_t timestamp);
        double getMean();
        double getMax();
        double getMin();
        double getStd();

        double getMean(simtime_t subwindow);
        double getMax(simtime_t subwindow);
        double getMin(simtime_t subwindow);
        double getStd(simtime_t subwindow);
        unsigned int getSize();
        void flush();
        int getFlushCounter();
        std::vector<Measurement> getSamples();
};

/**
 * Contains state variables ("TCB") for RDP.
 *
 * RdpStateVariables is effectively a "struct" -- it only contains
 * public data members. (Only declared as a class so that we can use
 * cObject as base class and make it possible to inspect
 * it in Tkenv.)
 *
 * RdpStateVariables only contains variables needed to implement
 * the "base" (RFC 793) RDP. More advanced RDP variants are encapsulated
 * into RdpAlgorithm subclasses which can have their own state blocks,
 * subclassed from RdpStateVariables. See RdpAlgorithm::createStateVariables().
 */
class INET_API RdpStateVariables : public cObject
{
public:
    RdpStateVariables();
    virtual std::string str() const override;
    virtual std::string detailedInfo() const OMNETPP5_CODE(override);

public:
    bool active;    // set if the connection was initiated by an active open
    int delayedNackNo;
    unsigned int request_id;
    unsigned int internal_request_id;
    double pacingTime;
    simtime_t lastPullTime;
    int IW;  //initial window size
    int cwnd;
    int ssthresh;
    int receivedPacketsInWindow;
    int sentPullsInWindow;
    int additiveIncreasePackets;
    int outOfWindowPackets;
    bool connFinished;
    int numPacketsToGet;
    int numPacketsToSend;
    bool congestionInWindow;
    bool slowStartState;
    bool waitToStart;
    int slowStartPacketsToSend;
    unsigned int numRcvdPkt;
    unsigned int numRcvTrimmedHeader;

    int numberReceivedPackets;
    int numberSentPackets;

    bool connNotAddedYet;
    bool isfinalReceivedPrintedOut;

    bool sendPulls;

    //Number of packets currently in flight. Inferred by IW and number of PR added
    int packetsInFlight;

    //RTT - Data
    simtime_t sRtt;
    simtime_t minRtt;
    simtime_t latestRtt;
    simtime_t rttvar;
    //RTT Step - Data
    simtime_t sRttStep;
    simtime_t minRttStep;
    simtime_t rttvarStep;

    //RTT - Header
    simtime_t sRttHeader;
    simtime_t minRttHeader;
    simtime_t latestRttHeader;
    simtime_t rttvarHeader;
    //RTT Step - Header
    simtime_t sRttStepHeader;
    simtime_t minRttStepHeader;
    simtime_t rttvarStepHeader;

    std::map<unsigned int, simtime_t> pullRequestsTransmissionTimes;

    simtime_t lastDataPacketArrived;

    Estimator bandwidthEstimator, rttPropEstimator;
};

class INET_API RdpConnection : public cSimpleModule
{
public:
    static simsignal_t cwndSignal;
    static simsignal_t trimmedHeadersSignal;
    struct PacketsToSend
    {
        unsigned int pktId;
        Packet *msg;
    };
    typedef std::list<PacketsToSend> PacketsList;
    PacketsList receivedPacketsList;
    // connection identification by apps: socketId
    int socketId = -1;    // identifies connection within the app
    int getSocketId() const
    {
        return socketId;
    }
    void setSocketId(int newSocketId)
    {
        ASSERT(socketId == -1);
        socketId = newSocketId;
    }

    int listeningSocketId = -1; // identifies listening connection within the app
    int getListeningSocketId() const
    {
        return listeningSocketId;
    }

    // socket pair
    L3Address localAddr;
    const L3Address& getLocalAddr() const
    {
        return localAddr;
    }
    L3Address remoteAddr;
    const L3Address& getRemoteAddr() const
    {
        return remoteAddr;
    }
    int localPort = -1;
    int remotePort = -1;
protected:
    Rdp *rdpMain = nullptr;    // RDP module

    // RDP state machine
    cFSM fsm;

    // variables associated with RDP state
    RdpStateVariables *state = nullptr;

    // RDP queues
    RdpSendQueue *sendQueue = nullptr;
    RdpSendQueue* getSendQueue() const
    {
        return sendQueue;
    }

    RdpReceiveQueue *receiveQueue = nullptr;
    RdpReceiveQueue* getReceiveQueue() const
    {
        return receiveQueue;
    }

public:
    virtual int getNumRcvdPackets();
    virtual bool isConnFinished();
    virtual void setConnFinished();

protected:
    cPacketQueue pullQueue;

    cMessage *paceTimerMsg;

    // RDP behavior in data transfer state
    RdpAlgorithm *rdpAlgorithm = nullptr;
    RdpAlgorithm* getRdpAlgorithm() const
    {
        return rdpAlgorithm;
    }

protected:
    /** @name FSM transitions: analysing events and executing state transitions */
    //@{
    /** Maps app command codes (msg kind of app command msgs) to RDP_E_xxx event codes */
    virtual RdpEventCode preanalyseAppCommandEvent(int commandCode);
    /** Implemements the pure RDP state machine */
    virtual bool performStateTransition(const RdpEventCode &event);
    /** Perform cleanup necessary when entering a new state, e.g. cancelling timers */
    virtual void stateEntered(int state, int oldState, RdpEventCode event);
    //@}

    /** @name Processing app commands. Invoked from processAppCommand(). */
    //@{
    virtual void process_OPEN_ACTIVE(RdpEventCode &event, RdpCommand *rdpCommand, cMessage *msg);
    virtual void process_OPEN_PASSIVE(RdpEventCode &event, RdpCommand *rdpCommand, cMessage *msg);

    /**
     * Process incoming RDP segment. Returns a specific event code (e.g. RDP_E_RCV_SYN)
     * which will drive the state machine.
     */
    virtual RdpEventCode process_RCV_SEGMENT(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address src, L3Address dest);
    virtual RdpEventCode processSegmentInListen(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address src, L3Address dest);

    virtual RdpEventCode processSegment1stThru8th(Packet *packet, const Ptr<const RdpHeader> &rdpseg);

    //@}
    /** Utility: clone a listening connection. Used for forking. */
    //virtual RdpConnection *cloneListeningConnection();
    //virtual void initClonedConnection(RdpConnection *listenerConn);
    /** Utility: creates send/receive queues and RdpAlgorithm */
    virtual void initConnection(RdpOpenCommand *openCmd);

    /** Utility: set snd_mss, rcv_wnd and sack in newly created state variables block */
    virtual void configureStateVariables();

    /** Utility: returns true if the connection is not yet accepted by the application */
    virtual bool isToBeAccepted() const
    {
        return listeningSocketId != -1;
    }
public:
    virtual void sendAckRdp(unsigned int AckNum); // MOH: HAS BEEN ADDED

    virtual void sendNackRdp(unsigned int nackNum); // MOH: HAS BEEN ADDED

    virtual void sendPacketToApp(unsigned int seqNum);

    virtual void prepareInitialRequest();

    virtual void closeConnection();

    virtual void sendInitialWindow();

    /** Utility: adds control info to segment and sends it to IP */
    virtual void sendToIP(Packet *packet, const Ptr<RdpHeader> &rdpseg);
    virtual void addRequestToPullsQueue();
    virtual void sendRequestFromPullsQueue();

    virtual int getPullsQueueLength();

    virtual void paceChanged(double newPace);

    /** Utility: start a timer */
    void scheduleTimeout(cMessage *msg, simtime_t timeout)
    {
        rdpMain->scheduleAt(simTime() + timeout, msg);
    }

protected:
    // /** Utility: cancel a timer */
    // cMessage* cancelEvent(cMessage *msg)
    // {
    //     return rdpMain->cancelEvent(msg);
    // }

    /** Utility: send IP packet */
    virtual void sendToIP(Packet *pkt, const Ptr<RdpHeader> &rdpseg, L3Address src, L3Address dest);

    /** Utility: sends packet to application */
    virtual void sendToApp(cMessage *msg);

    /** Utility: sends status indication (RDP_I_xxx) to application */
    virtual void sendIndicationToApp(int code, const int id = 0);

    /** Utility: sends RDP_I_ESTABLISHED indication with RDPConnectInfo to application */
    virtual void sendEstabIndicationToApp();

public:
    /** Utility: prints local/remote addr/port and app gate index/connId */
    virtual void printConnBrief() const;
    /** Utility: prints important header fields */
    static void printSegmentBrief(Packet *packet, const Ptr<const RdpHeader> &rdpseg);
    /** Utility: returns name of RDP_S_xxx constants */
    static const char* stateName(int state);
    /** Utility: returns name of RDP_E_xxx constants */
    static const char* eventName(int event);
    /** Utility: returns name of RDP_I_xxx constants */
    static const char* indicationName(int code);

public:
    RdpConnection()
    {
    }
    RdpConnection(const RdpConnection &other)
    {
    }
    void initialize()
    {
    }

    /**
     * The "normal" constructor.
     */
    virtual void initConnection(Rdp *mod, int socketId);

    /**
     * Destructor.
     */
    virtual ~RdpConnection();

    int getLocalPort() const
    {
        return localPort;
    }
    L3Address getLocalAddress() const
    {
        return localAddr;
    }

    int getRemotePort() const
    {
        return remotePort;
    }
    L3Address getRemoteAddress() const
    {
        return remoteAddr;
    }

    virtual void segmentArrivalWhileClosed(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address src, L3Address dest);

    /** @name Various getters **/
    //@{
    int getFsmState() const
    {
        return fsm.getState();
    }
    RdpStateVariables* getState()
    {
        return state;
    }
    RdpSendQueue* getSendQueue()
    {
        return sendQueue;
    }
    RdpReceiveQueue* getReceiveQueue()
    {
        return receiveQueue;
    }
    RdpAlgorithm* getRdpAlgorithm()
    {
        return rdpAlgorithm;
    }
    Rdp* getRDPMain()
    {
        return rdpMain;
    }

    virtual bool processTimer(cMessage *msg);

    virtual void activatePullTimer();

    virtual void schedulePullTimer(double time);

    virtual bool processrdpsegment(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address srcAddr, L3Address destAddr);

    virtual bool processAppCommand(cMessage *msg);

    virtual void handleMessage(cMessage *msg);

    virtual void computeRtt(unsigned int pullSeqNum, bool isHeader);
    virtual void rttMeasurementComplete(simtime_t newRtt, bool isHeader);

    virtual void cancelRequestTimer();

    /**
     * Utility: converts a given simtime to a timestamp (TS).
     */
    static uint32 convertSimtimeToTS(simtime_t simtime);

    /**
     * Utility: converts a given timestamp (TS) to a simtime.
     */
    static simtime_t convertTSToSimtime(uint32 timestamp);

};

} // namespace RDP

} // namespace inet

#endif // ifndef __INET_RdpConnection_H

