#ifndef __SIMULATION_RUNNER_H_
#define __SIMULATION_RUNNER_H_

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <iostream>
#include <string>
#include <math.h>
#include <array>
#include <random>
#include <tuple>
#include <omnetpp.h>
#include <cmdenv/cmddefs.h>
#include <cmdrlenv.h>
#include <envir/sectionbasedconfig.h>
#include <envir/inifilereader.h>
#include <Broker.h>
#include <unordered_map>
#include <sim/netbuilder/cnedloader.h>
#include <omnetpp/globals.h>

using namespace std;
using namespace omnetpp;

class SimulationRunner{

    public:

        Cmdrlenv *env;
        cSimulation *simulationPtr;
        cEvent *event;
        cStaticFlag dummy;
        SectionBasedConfiguration *bootconfigptr;
        InifileReader *inifilePtr;
        bool needsCleaning = false;

        SimulationRunner();

        void initialise(std::string inipath);
        std::unordered_map<std::string, ObsType > reset();
        // std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string, bool > > step(ActionType action);
        std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string, bool > > step(std::unordered_map<std::string, ActionType > actions);

        void cleanupmemory();

        void shutdown();
};

#endif
