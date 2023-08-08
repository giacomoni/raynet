package simulations.RLSimulations;

@namespace(inet);
@namespace(rdp);


import rdp.node.StandardHostRdp;
import ecmp.networklayer.configurator.ipv4.Ipv4NetworkConfiguratorEcmp;
import inet.node.inet.Router;
import rdp.base.RouterRdp;
import ned.DatarateChannel;
import ned.IBidirectionalChannel;
import RLComponents.Broker;
import RLComponents.Stepper;

network simplenetwork
{
    parameters:
        @display("bgb=512,395");
        int numberOfClients = default(1);
        int numberOfNormalFlows = default(1);
        int numberOfLongerFlows = default(0);
        volatile double minDelay @unit(s);
        volatile double linkrate @unit(Mbps);

    types:
        channel ethernetline extends DatarateChannel
        {
            delay = parent.minDelay;
            datarate = parent.linkrate;
        }
    submodules:
        client[numberOfClients]: StandardHostRdp {
            @display("p=162,225,m,n,$numberOfClients,150");
        }
        configurator: Ipv4NetworkConfiguratorEcmp {
            @display("p=450,350");
        }
        server1: StandardHostRdp {
            @display("p=464,117");
        }

        broker: Broker;
        stepper: Stepper;
    connections:
        client[0].pppg++ <--> ethernetline <--> server1.pppg++;
}
