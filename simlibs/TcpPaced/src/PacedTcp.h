//
// Copyright (C) 2020 Luca Giacomoni and George Parisis
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

#ifndef SRC_TRANSPORTLAYER_PACEDTCP_H_
#define SRC_TRANSPORTLAYER_PACEDTCP_H_

#include <inet/transportlayer/tcp/Tcp.h>
#include <inet/transportlayer/tcp/TcpConnection.h>

using namespace inet::tcp;
using namespace omnetpp;
/*
 * Overrides Tcp implementation. Right now it is only used to define new NED parameters.
 */
class PacedTcp : public Tcp
{
public:
    PacedTcp();
    virtual ~PacedTcp();

protected:
    /** Factory method; may be overriden for customizing Tcp */
    virtual TcpConnection* createConnection(int socketId);
};

#endif /* SRC_TRANSPORTLAYER_PACEDTCP_H_ */
