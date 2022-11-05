//
// Copyright (C) 2004 Andras Varga
// Copyright (C) 2009-2010 Thomas Reschka
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_RdpAlgorithm_H
#define __INET_RdpAlgorithm_H

#include <inet/common/INETDefs.h>

#include "../../transportlayer/rdp/rdp_common/RdpHeader.h"
#include "../rdp/RdpConnection.h"

namespace inet {

namespace rdp {

/**
 * Abstract base class for RDP algorithms which encapsulate all behaviour
 * during data transfer state: flavour of congestion control, fast
 * retransmit/recovery, selective acknowledgement etc. Subclasses
 * may implement various sets and flavours of the above algorithms.
 */
class INET_API RdpAlgorithm : public cObject
{
protected:
    RdpConnection *conn;    // we belong to this connection
    RdpStateVariables *state;    // our state variables

    /**
     * Create state block (TCB) used by this RDP variant. It is expected
     * that every RdpAlgorithm subclass will have its own state block,
     * subclassed from RdpStateVariables. This factory method should
     * create and return a "blank" state block of the appropriate type.
     */
    virtual RdpStateVariables* createStateVariables() = 0;

public:
    /**
     * Ctor.
     */
    RdpAlgorithm()
    {
        state = nullptr;
        conn = nullptr;
    }

    /**
     * Virtual dtor.
     */
    virtual ~RdpAlgorithm()
    {
    }

    /**
     * Assign this object to a RdpConnection. Its sendQueue and receiveQueue
     * must be set already at this time, because we cache their pointers here.
     */
    void setConnection(RdpConnection *_conn)
    {
        conn = _conn;
    }

    /**
     * Creates and returns the RDP state variables.
     */
    RdpStateVariables* getStateVariables()
    {
        if (!state)
            state = createStateVariables();

        return state;
    }

    /**
     * Should be redefined to initialize the object: create timers, etc.
     * This method is necessary because the RdpConnection ptr is not
     * available in the constructor yet.
     */
    virtual void initialize()
    {
    }

    /**
     * Called when the connection closes, it should cancel all running timers.
     */
    virtual void connectionClosed() = 0;

    /**
     * Place to process timers specific to this RdpAlgorithm class.
     * RdpConnection will invoke this method on any timer (self-message)
     * it doesn't recognize (that is, any timer other than the 2MSL,
     * CONN-ESTAB and FIN-WAIT-2 timers).
     *
     * Method may also change the event code (by default set to RDP_E_IGNORE)
     * to cause the state transition of RDP FSM.
     */
    virtual void processTimer(cMessage *timer, RdpEventCode &event) = 0;

    virtual void dataSent(uint32 fromseq) = 0;

    virtual void ackSent() = 0;

    virtual void receivedHeader(unsigned int seqNum) = 0;

    virtual void receivedData(unsigned int seqNum) = 0;


};

} // namespace RDP

} // namespace inet

#endif // ifndef __INET_RdpAlgorithm_H

