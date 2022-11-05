

package rdp.node;

import inet.transportlayer.contract.ITcp;
import inet.applications.contract.IApp;
import inet.transportlayer.contract.ISctp;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import inet.applications.tunapp.TunnelApp;

import rdp.node.ApplicationLayerNodeBaseEcmp;
import ecmp.networklayer.ipv4.Ipv4NetworkLayerEcmp;
import rdp.transportlayer.contract.IRdp;

module StandardHostRdp extends ApplicationLayerNodeBaseEcmp
{
    parameters:
        @display("i=device/pc2");
        //@figure[submodules];
		forwarding = default(false);
        *.routingTableModule = default("^.ipv4.routingTable");

}
