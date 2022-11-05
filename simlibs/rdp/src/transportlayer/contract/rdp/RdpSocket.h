#ifndef __NDP_RdpSocket_H
#define __NDP_RdpSocket_H

#include <inet/common/INETDefs.h>
#include <inet/common/packet/ChunkQueue.h>
#include <inet/common/packet/Message.h>
#include <inet/common/packet/Packet.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/common/socket/ISocket.h>
#include "../rdp/RdpCommand_m.h"

namespace inet {

class RdpStatusInfo;

class INET_API RdpSocket : public ISocket
{
public:
    /**
     * Abstract base class for your callback objects. See setCallbackObject()
     * and processMessage() for more info.
     *
     * Note: this class is not subclassed from cObject, because
     * classes may have both this class and cSimpleModule as base class,
     * and cSimpleModule is already a cObject.
     */
    class INET_API ICallback
    {
    public:
        virtual ~ICallback()
        {
        }
        /**
         * Notifies about data arrival, packet ownership is transferred to the callee.
         */
        virtual void socketDataArrived(RdpSocket *socket, Packet *packet, bool urgent) = 0;
        virtual void socketAvailable(RdpSocket *socket, RdpAvailableInfo *availableInfo) = 0;
        virtual void socketEstablished(RdpSocket *socket) = 0;
        virtual void socketPeerClosed(RdpSocket *socket) = 0;
        virtual void socketClosed(RdpSocket *socket) = 0;
        virtual void socketFailure(RdpSocket *socket, int code) = 0;
        virtual void socketStatusArrived(RdpSocket *socket, RdpStatusInfo *status) = 0;
        virtual void socketDeleted(RdpSocket *socket) = 0;
    };

    enum State
    {
        NOT_BOUND, BOUND, LISTENING, CONNECTING, CONNECTED
    };

protected:
    int connId = -1;
    State sockstate = NOT_BOUND;

    L3Address localAddr;
    int localPrt = -1;
    L3Address remoteAddr;
    int remotePrt = -1;

    ICallback *cb = nullptr;
    cGate *gateToRdp = nullptr;
    std::string rdpAlgorithmClass;

protected:
    void sendToRDP(cMessage *msg, int c = -1);

    // TODO: in the future to support apps with multiple incoming connections
    void listen(bool fork);

public:
    /**
     * Constructor. The getConnectionId() method returns a valid Id right after
     * constructor call.
     */
    RdpSocket();

    /**
     * Destructor
     */
    ~RdpSocket();

    /**
     * Returns the internal connection Id. NDP uses the (gate index, connId) pair
     * to identify the connection when it receives a command from the application
     * (or RdpSocket).
     */
    int getSocketId() const override
    {
        return connId;
    }
    /**
     * Returns the socket state, one of NOT_BOUND, CLOSED, LISTENING, CONNECTING,
     * CONNECTED, etc. Messages received from NDP must be routed through
     * processMessage() in order to keep socket state up-to-date.
     */
    RdpSocket::State getState()
    {
        return sockstate;
    }

    /**
     * Returns name of socket state code returned by getState().
     */
    static const char* stateName(RdpSocket::State state);

    void setState(RdpSocket::State state)
    {
        sockstate = state;
    }
    ;

    /** @name Getter functions */
    //@{
    L3Address getLocalAddress()
    {
        return localAddr;
    }
    int getLocalPort()
    {
        return localPrt;
    }
    L3Address getRemoteAddress()
    {
        return remoteAddr;
    }
    int getRemotePort()
    {
        return remotePrt;
    }
    //@}

    /** @name Opening and closing connections, sending data */
    //@{
    /**
     * Sets the gate on which to send to NDP. Must be invoked before socket
     * can be used. Example: <tt>socket.setOutputGate(gate("raptondpOut"));</tt>
     */
    void setOutputGate(cGate *toRdp)
    {
        gateToRdp = toRdp;
    }

    /**
     * Binds the provided local address and port to the RdpSocket.
     */
    void bind(L3Address localAddr, int localPort);

    /**
     * Returns the current ndpAlgorithmClass parameter.
     */
    const char* getNdpAlgorithmClass() const
    {
        return rdpAlgorithmClass.c_str();
    }

    /**
     * Sets the ndpAlgorithmClass parameter of the next connect() or listen() call.
     */
    void setRdpAlgorithmClass(const char *rdpAlgorithmClass)
    {
        this->rdpAlgorithmClass = rdpAlgorithmClass;
    }

    /**
     * Initiates passive OPEN, creating a "forking" connection that will listen
     * on the port you bound the socket to. Every incoming connection will
     * get a new connId (and thus, must be handled with a new RdpSocket object),
     * while the original connection (original connId) will keep listening on
     * the port. The new RdpSocket object must be created with the
     * RdpSocket(cMessage *msg) constructor.
     *
     * If you need to handle multiple incoming connections, the RdpSocketMap
     * class can also be useful, and NDPSrvHostApp shows how to put it all
     * together. See also NDPOpenCommand documentation (neddoc) for more info.
     */
    void listen()
    {
        listen(true);
    }

    /**
     * Initiates passive OPEN to create a non-forking listening connection.
     * Non-forking means that NDP will accept the first incoming
     * connection, and refuse subsequent ones.
     *
     * See NDPOpenCommand documentation (neddoc) for more info.
     */
    void listenOnce()
    {
        listen(false);
    }

    /**
     * NOT USED
     * Not used due as an NDP server does not need to accept an attempt to create a new connection unlike TCP.
     * The connection instead is immediately made in the simulation of NDP.
     *
     * TODO: maybe create in the future applications that can accept multiple incoming NDP connections
     */
    void accept(int socketId);

    /**
     * Connect to the a remote socket given its remote address and port. The localAddress is also
     * used to immediately know the local socket address without the need of waiting for a syn/ack
     * as with the TCP implementation. The local port is assigned by the transport layer see Ndp::getEphemeralPort()
     * numPacketsToSend needed to fill sendQueue with number of packets to send to the receiver.
     */
    void connect(L3Address localAddress, L3Address remoteAddr, int remotePort, unsigned int numPacketsToSend);

    /**
     * NOT USED
     * Not implemented. The client never sends any packets to the ndp connection as this is done
     * within the connection itself. To improve the client should send the packets from the application
     * itself using this method.
     */
    void send(Packet *msg);

    /**
     * NOT USED
     * This method is never called as NDP never sends a CLOSE operation
     * to the socket.
     */
    void close() override;

    /**
     * NOT USED
     * This is never called as a ABORT command is never sent to the socket
     * as there is no retransmission limit which can be exceeded in the NDP
     * implementation unlike TCP.
     */
    void abort();

    /**
     * * NOT USED
     * The handleCrashOperation method is never handled by the NDP socket,
     * therefore the socket is never destroyed.
     */
    virtual void destroy() override;

    /**
     * NOT USED
     * This is not needed as the socket will never be closed unless the simulation
     * is completed. TODO Allow the possibility of re-connecting with an already
     * established RdpSocket object. The renewSocket is used to ensure that the socket has not been
     * closed or aborted. If this is the case, the socket will be renewed rather
     * than creating an entirely new connectionId (see NdpAppBase::connect).
     */
    void renewSocket();

    // Not used as this is used to verify that the socket is not open when trying to
    // close, but the socket does not close within this NDP implementation.
    virtual bool isOpen() const override;

    /**
     * Returns true if the message belongs to this socket instance (message
     * has a NDPCommand as getControlInfo(), and the connId in it matches
     * that of the socket.)
     */
    bool belongsToSocket(cMessage *msg) const override;

    /**
     * Sets a callback object, to be used with processMessage().
     * This callback object may be your simple module itself (if it
     * multiply inherits from CallbackInterface too, that is you
     * declared it as
     * <pre>
     * class MyAppModule : public cSimpleModule, public RdpSocket::CallbackInterface
     * </pre>
     * and redefined the necessary virtual functions; or you may use
     * dedicated class (and objects) for this purpose.
     *
     * RdpSocket doesn't delete the callback object in the destructor
     * or on any other occasion.
     *
     * YourPtr is an optional pointer. It may contain any value you wish --
     * RdpSocket will not look at it or do anything with it except passing
     * it back to you in the CallbackInterface calls. You may find it
     * useful if you maintain additional per-connection information:
     * in that case you don't have to look it up by connId in the callbacks,
     * you can have it passed to you as yourPtr.
     */
    void setCallback(ICallback *cb);

    /**
     * Examines the message (which should have arrived from NDP),
     * updates socket state, and if there is a callback object installed
     * (see setCallbackObject(), class CallbackInterface), dispatches
     * to the appropriate method of it with the same yourPtr that
     * you gave in the setCallbackObject() call.
     *
     * The method deletes the message, unless (1) there is a callback object
     * installed AND (2) the message is payload (message kind NDP_I_DATA) when
     *  the responsibility of destruction is on the socketDataArrived() callback
     *  method.
     *
     * IMPORTANT: for performance reasons, this method doesn't check that
     * the message belongs to this socket, i.e. belongsToSocket(msg) would
     * return true!
     */
    void processMessage(cMessage *msg) override;
    //@}
};

} // namespace inet

#endif // ifndef __INET_RdpSocket_H

