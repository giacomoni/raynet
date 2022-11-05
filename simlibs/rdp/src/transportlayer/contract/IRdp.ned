

package rdp.transportlayer.contract;
 
moduleinterface IRdp
{
    @display("i=block/wheelbarrow");
    gates:
        input appIn @labels(RDPCommand/down);
        input ipIn @labels(RdpHeader,IPv4ControlInfo/up,IPv6ControlInfo/up);
        output appOut @labels(RDPCommand/up);
        output ipOut @labels(RdpHeader,IPv4ControlInfo/down,IPv6ControlInfo/down);
}
