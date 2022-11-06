

package simulations.SimpleSimulation;

@namespace(inet);
@namespace(ndp);


import rdp.node.StandardHostRdp;
import ecmp.networklayer.configurator.ipv4.Ipv4NetworkConfiguratorEcmp;
import inet.node.inet.Router;
import rdp.base.RouterRdp;
import ned.DatarateChannel;
import ned.IBidirectionalChannel;

network simplenetwork
{
    parameters:
    	@display("bgb=512,395");
    	int numberOfClients = default(2);
    	int numberOfNormalFlows = default(1);
    	int numberOfLongerFlows = default(0);
        int numberOfRouters = default(1);
    types:
        channel ethernetline extends DatarateChannel
        {
            delay =  25ms;
            datarate = 10Mbps;
        }
    submodules:
        client[numberOfClients]: StandardHostRdp {
            @display("p=44,50,m,n,$numberOfClients,150");
        }
        configurator: Ipv4NetworkConfiguratorEcmp {
            @display("p=450,350");
        }
        server: StandardHostRdp {
            @display("p=431,50");
        }
        router[numberOfRouters]: RouterRdp {
            @display("p=252,50,m,n,$numberOfRouters,150");
        }
    connections:
       for i=0..sizeof(client)-1, for j=0..sizeof(router)-1 {
            client[i].pppg++ <--> ethernetline <--> router[j].pppg++;
        }
        for i=0..sizeof(router)-1{
            router[i].pppg++ <--> ethernetline <--> server.pppg++;
        }
} 
