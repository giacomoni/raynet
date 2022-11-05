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

package ecmp.networklayer.configurator.ipv4;

import inet.networklayer.configurator.base.NetworkConfiguratorBase;
import ecmp.networklayer.configurator.base.NetworkConfiguratorBaseEcmp;

simple Ipv4NetworkConfiguratorEcmp extends NetworkConfiguratorBaseEcmp
{
    parameters:
        @class("inet::Ipv4NetworkConfiguratorEcmp");
        @display("i=block/cogwheel");
        xml config = default(xml("<config><interface hosts='**' address='10.x.x.x' netmask='255.x.x.x'/></config>")); // XML configuration parameters for IP address assignment and adding manual routes
        bool assignAddresses = default(true); // assign IP addresses to all interfaces in the network
        bool assignUniqueAddresses = default(true); // avoid using the same address and raise an error if not possible
        bool assignDisjunctSubnetAddresses = default(true); // avoid using the same address prefix and netmask on different links when assigning IP addresses to interfaces
        bool addStaticRoutes = default(true);  // add static routes to the routing tables of all nodes to route to all destination interfaces (only where applicable; turn off when config file contains manual routes)
        bool addDefaultRoutes = default(true); // add default routes if all routes from a source node go through the same gateway (used only if addStaticRoutes is true)
        bool addSubnetRoutes = default(true);  // add subnet routes instead of destination interface routes (only where applicable; used only if addStaticRoutes is true)
        bool addDirectRoutes = default(true);  // add direct routes (i.e. directly connected interfaces) to the routing table (used only if addStaticRoutes is true)
        bool optimizeRoutes = default(true); // optimize routing tables by merging routes, the resulting routing table might route more packets than the original (used only if addStaticRoutes is true)
        bool dumpTopology = default(false);  // print extracted network topology to the module output
        bool dumpLinks = default(false);     // print recognized network links to the module output
        bool dumpAddresses = default(false); // print assigned IP addresses for all interfaces to the module output
        bool dumpRoutes = default(false);    // print configured and optimized routing tables for all nodes to the module output
        string dumpConfig = default("");     // write configuration into the given config file that can be fed back to speed up subsequent runs (network configurations)
}

