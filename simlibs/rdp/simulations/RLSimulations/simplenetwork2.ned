package rdp.simulations.RLSimulations;

@namespace(inet);
@namespace(rdp);


import rdp.node.StandardHostRdp;
import ecmp.networklayer.configurator.ipv4.Ipv4NetworkConfiguratorEcmp;
import inet.node.inet.Router;
import rdp.base.RouterRdp;
import ned.DatarateChannel;
import ned.IBidirectionalChannel;
import model.Broker;
import model.Stepper;

network simplenetwork2
{
    parameters:
        @display("bgb=512,395");
        int numberOfClients = default(5);
        int numberOfNormalFlows = default(1);
        int numberOfLongerFlows = default(0);
        int numberOfRouters = default(1);
        volatile double minDelay @unit(s);
        volatile double linkrate @unit(Mbps);

    types:
        channel ethernetline extends DatarateChannel
        {
            delay = minDelay;
            datarate = linkrate;
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
        server2: StandardHostRdp {
            @display("p=340,23");
        }
        router1: RouterRdp {
            @display("p=162,117,m,n,$numberOfRouters,150");
        }
        router2: RouterRdp {
            @display("p=340,117,m,n,$numberOfRouters,150");
        }
        broker: Broker;
        stepper: Stepper;
    connections:
        client[0].pppg++ <--> ethernetline <--> router1.pppg++;
        client[1].pppg++ <--> ethernetline <--> router1.pppg++;
        router1.pppg++ <--> ethernetline <--> router2.pppg++;
        client[2].pppg++ <--> ethernetline <--> router2.pppg++;
        client[3].pppg++ <--> ethernetline <--> router2.pppg++;
        client[4].pppg++ <--> ethernetline <--> router2.pppg++;
        router2.pppg++ <--> ethernetline <--> server1.pppg++;
        router2.pppg++ <--> ethernetline <--> server2.pppg++;
}
