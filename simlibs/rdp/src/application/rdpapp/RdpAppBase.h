#ifndef __RDP_RAPTORdpAppBase_H
#define __RDP_RAPTORdpAppBase_H

#include <inet/common/INETDefs.h>
#include <inet/applications/base/ApplicationBase.h>
#include "../../transportlayer/contract/rdp/RdpSocket.h"

namespace inet {

/**
 * Base class for clients app for RDP-based request-reply protocols or apps.
 * Handles a single session (and RDP connection) at a time.
 *
 * It needs the following NED parameters: localAddress, localPort, connectAddress, connectPort.
 */
class INET_API RdpAppBase : public ApplicationBase, public RdpSocket::ICallback
{
protected:
    RdpSocket socket;

protected:
    // Initializes the application, binds the socket to the local address and port.
    virtual void initialize(int stage) override;

    virtual int numInitStages() const override
    {
        return NUM_INIT_STAGES;
    }

    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

    /* Utility functions */
    // Creates a socket connection based on the NED parameters specified.
    virtual void connect();
    // Closes the socket.
    virtual void close();

    /* RdpSocket::ICallback callback methods */
    virtual void handleTimer(cMessage *msg) = 0;

    // Called once the socket is established. Currently does nothing but set the status string.
    virtual void socketEstablished(RdpSocket *socket) override;

    //Called once a packet arrives at the application.
    virtual void socketDataArrived(RdpSocket *socket, Packet *msg, bool urgent) override;

    virtual void socketAvailable(RdpSocket *socket, RdpAvailableInfo *availableInfo) override
    {
        socket->accept(availableInfo->getNewSocketId());
    }
    virtual void socketPeerClosed(RdpSocket *socket) override;
    virtual void socketClosed(RdpSocket *socket) override;
    virtual void socketFailure(RdpSocket *socket, int code) override;
    virtual void socketStatusArrived(RdpSocket *socket, RdpStatusInfo *status)
    override
    {
        delete status;
    }
    virtual void socketDeleted(RdpSocket *socket) override
    {
    }
};

} // namespace inet

#endif // ifndef __INET_RAPTORdpAppBase_H

