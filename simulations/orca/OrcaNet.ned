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

package simulations.orca;


import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.StandardHost;
import ned.DatarateChannel;
import RLComponents.Broker;
import RLComponents.Stepper;

channel coreChannel extends DatarateChannel
{
    datarate = default(1 Gbps);
    delay = default(0ms);
    ber = default(0);
    per = default(0.0);
    
}


//an instance of the Broker module and Sender module that communicates through signals
network OrcaNet
{
 parameters:
    
    submodules:
        broker: Broker;
        stepper: Stepper;
        client: StandardHost {
            parameters:
                @display("p=250,100");
            gates:
                pppg[1];
        }
        server: StandardHost {
            parameters:
                @display("p=400,100;i=device/server");
            gates:
                pppg[1];
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=100,100;is=s");
        }

        
    connections:
        client.pppg[0] <--> coreChannel <--> server.pppg[0];
       
}
        
   



