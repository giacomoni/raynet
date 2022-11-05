package rdp.transportlayer.rdp;

import rdp.transportlayer.contract.IRdp;

simple Rdp like IRdp
{
    parameters:
        int initialWindow = default(2);  // initialWindow size RDP-style (sender pushes IW pacakets)
        int ssthresh = default(50);
        int additiveIncreasePackets = default(1);
        string rdpAlgorithmClass = default("RdpAIMD");
        double thrWeight = default(1.0);
        double trimWeight = default(2.0);
        double delayWeight = default(1.0);
        @display("i=block/wheelbarrow");

        @signal[packetReceivedFromUpper](type=cPacket);
        @signal[packetReceivedFromLower](type=cPacket);
    gates:
        input appIn @labels(RDPCommand/down) @messageKinds(inet::RdpCommandCode);
        input ipIn @labels(RdpHeader,Ipv4ControlInfo/up,Ipv6ControlInfo/up);
        output appOut @labels(RDPCommand/up) @messageKinds(inet::RdpStatusInd);
        output ipOut @labels(RdpHeader,Ipv4ControlInfo/down,Ipv6ControlInfo/down);
}

