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

package ecmp.networklayer.ipv4;

import inet.common.MessageDispatcher;
import inet.networklayer.configurator.contract.IIpv4NodeConfigurator;
import inet.networklayer.configurator.ipv4.Ipv4NodeConfigurator;
import inet.networklayer.contract.IArp;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.ipv4.Ipv4NatTable;
import inet.networklayer.ipv4.IIgmp;
import inet.networklayer.ipv4.Icmp;
import inet.networklayer.ipv4.Icmp;
import inet.networklayer.ipv4.Igmpv2;
import inet.networklayer.ipv4.IIpv4;

import ecmp.networklayer.ipv4.IIpv4Ecmp;
import ecmp.networklayer.configurator.ipv4.Ipv4NodeConfiguratorEcmp;
import ecmp.networklayer.ipv4.Ipv4RoutingTableEcmp;
//
// Network layer of an IPv4 node.
//
//
module Ipv4NetworkLayerEcmp like INetworkLayer
{
    parameters:
        @class("inet::Ipv4NetworkLayerEcmp");
        bool forwarding = default(false);
        bool multicastForwarding = default(false);
        string interfaceTableModule;
        string displayStringTextFormat = default("%i");
        *.forwarding = forwarding;
        *.multicastForwarding = multicastForwarding;
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        *.routingTableModule = default(absPath(".routingTable"));
        *.arpModule = default(absPath(".arp"));
        *.icmpModule = default(absPath(".icmp"));
        @class(::inet::Ipv4NetworkLayer);
        @display("i=block/fork");

    gates:
        input ifIn @labels(INetworkHeader);
        output ifOut @labels(INetworkHeader);
        input transportIn @labels(Ipv4ControlInfo/down);
        output transportOut @labels(Ipv4ControlInfo/up);

    submodules:
        configurator: <default("Ipv4NodeConfiguratorEcmp")> like IIpv4NodeConfigurator if typename != "" {
            parameters:
                @display("p=100,100;is=s");
        }
        routingTable: Ipv4RoutingTableEcmp {
            parameters:
                @display("p=100,200;is=s");
        }
        natTable: Ipv4NatTable {
            parameters:
                @display("p=100,300;is=s");
        }
        up: MessageDispatcher {
            parameters:
                @display("p=550,100;b=600,5");
        }
        igmp: Igmpv2 {
            parameters:
                @display("p=400,200");
        }
        icmp: Icmp {
            parameters:
                @display("p=700,200");
        }
        mp: MessageDispatcher {
            parameters:
                @display("p=550,300;b=600,5");
        }
        ip: <default("Ipv4Ecmp")> like IIpv4Ecmp {
            parameters:
                @display("p=700,400;q=queue");
        }
        arp: <default("Arp")> like IArp {
            parameters:
                @display("p=400,400;q=pendingQueue");
        }
        lp: MessageDispatcher {
            parameters:
                @display("p=550,500;b=600,5");
        }

    connections allowunconnected:
        transportIn --> { @display("m=n"); } --> up.in++;
        transportOut <-- { @display("m=n"); } <-- up.out++;

        up.out++ --> igmp.routerIn;
        up.in++ <-- igmp.routerOut;

        up.out++ --> mp.in++;
        up.in++ <-- mp.out++;

        up.out++ --> icmp.transportIn;
        up.in++ <-- icmp.transportOut;

        igmp.ipOut --> mp.in++;
        igmp.ipIn <-- mp.out++;

        icmp.ipOut --> mp.in++;
        icmp.ipIn <-- mp.out++;

        mp.out++ --> ip.transportIn;
        mp.in++ <-- ip.transportOut;

        arp.ifOut --> lp.in++;
        arp.ifIn <-- lp.out++;

        ip.queueOut --> lp.in++;
        ip.queueIn <-- lp.out++;

        lp.out++ --> { @display("m=s"); } --> ifOut;
        lp.in++ <-- { @display("m=s"); } <-- ifIn;
}

