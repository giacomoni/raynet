//
// Copyright (C) 2004 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//





package simulations.RLSimulations;

import inet.common.misc.ThruputMeteringChannel;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import RLComponents.Broker;
import RLComponents.Stepper;



network Dumbell
{
    parameters:
        int n;
    types:

        channel ethernetline extends ThruputMeteringChannel
        {
            delay = 41ms;
            datarate = 128Mbps;
        }
        
        channel fiberline extends ThruputMeteringChannel
        {
            delay = 0ms;
            datarate = 1000Gbps;
        }

    submodules:
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=100,100;is=s");
        }
        r1: Router {
            @display("p=400,250");
        }
        r2: Router {
            @display("p=577,250");
        }
        cli[n]: StandardHost {
            parameters:
                @display("i=device/laptop;p=250,100,col,100");
        }
        srv[n]: StandardHost {
            parameters:
                @display("i=device/server;p=850,250");
        }

        broker: Broker{}
        stepper: Stepper{}
    connections:
        for i=0..n-1 {
            cli[i].pppg++ <--> fiberline <--> r1.pppg++;
        }
        for i=0..n-1 {
            srv[i].pppg++ <--> fiberline <--> r2.pppg++;
        }

        r2.pppg++ <--> ethernetline <--> r1.pppg++;
}

