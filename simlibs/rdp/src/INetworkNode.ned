
package rdp;

//
// Contains the common interface for all node types in a network.
//
moduleinterface INetworkNode
{
    parameters:
        @display("bgb=,448");
        @networkNode;
        @labels(node,ethernet-node,wireless-node);
    gates:
        input radioIn[numRadios] @directIn;
        inout pppg[] @labels(PPPFrame-conn);
        inout ethg[] @labels(EtherFrame-conn);
}
