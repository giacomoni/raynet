//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package mltcp.transportlayer;

import inet.transportlayer.tcp.TcpConnection;

//
// Same behaviour as TcpConnection. We define a new NED module to declare signals used to rcord statistics.
//
simple TcpConnectionResultsRecording extends TcpConnection
{
    parameters:
        @class(PacedTcpConnection);
        
        @signal[throughput];
        @signal[action];
        @signal[dupAcks];
        @signal[rttGradient];
        @signal[tick];
        @signal[MIQueueSize];
        
        @statistic[throughput](record=vector;);
        @statistic[action](record=vector;);
        @statistic[dupAcks](record=vector;);
        @statistic[rttGradient](record=vector;);
        @statistic[tick](record=vector;);
        @statistic[MIQueueSize](record=vector;);
}
