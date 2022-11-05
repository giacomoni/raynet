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

package rdp.common.queue;

import inet.queueing.base.PacketQueueBase;
import inet.queueing.contract.IPacketQueue;

simple RdpSwitchQueue extends PacketQueueBase like IPacketQueue
{
    parameters:
        @class("inet::queueing::RdpSwitchQueue");
        int packetCapacity = default(8); //= default(100);
        string queueNameData = default("L2DataQueue"); // name of the inner cQueue object, used in the 'q' tag of the display string
        string queueNameHeader = default("L2HeadersQueue"); // name of the inner cQueue object, used in the 'q' tag of the display string
        
        
        
        @display("i=block/queue");
        @signal[packetPushed](type=inet::Packet);//<- @signal[rcvdPk](type=inet::Packet);
        @signal[packetPopped](type=inet::Packet);
        @signal[packetRemoved](type=inet::Packet);
        @signal[packetDropped](type=inet::Packet);
        
        @signal[dataQueueLength](type=long);
        @signal[headersQueueLength](type=long);
        @signal[numTrimmedPkt](type=long);
        
        @statistic[packetPushed](title="packets pushed"; source=localSignal(packetPushed); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetPopped](title="packets popped"; source=localSignal(packetPopped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetRemoved](title="packets removed"; source=localSignal(packetRemoved); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        @statistic[packetDropped](title="packets dropped: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
        
        @statistic[queueingTime](title="queueing times"; source=queueingTime(packetPopped); record=histogram,vector; unit=s; interpolationmode=none);
        
        //@statistic[rcvdPk](title="received packets"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        //@statistic[dropPk](title="dropped packets"; source=dropPkByQueue; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        //@statistic[queueingTime](title="queueing time"; record=histogram,vector; interpolationmode=none);
        @statistic[dataQueueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);
        @statistic[headersQueueLength](title="queue length"; record=max,timeavg,vector; interpolationmode=sample-hold);
        @statistic[numTrimmedPkt](title="num Trimmed Pkt  "; source=numTrimmedPkt; record=last ; interpolationmode=none);
}