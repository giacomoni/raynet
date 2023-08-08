#ifndef __GYM_API_H_
#define __GYM_API_H_

#include <iostream>
#include <string>
#include <math.h>
#include <array>
#include <random>
#include <tuple>
#include <omnetpp.h>
#include <cmdenv/cmddefs.h>
#include <Cmdrlenv.h>
#include <envir/sectionbasedconfig.h>
#include <envir/inifilereader.h>
#include <Broker.h>
#include <unordered_map>
#include <sim/netbuilder/cnedloader.h>
#include <omnetpp/globals.h>

using namespace std;
using namespace omnetpp;

class GymApi{

    public:

        Cmdrlenv *env;
        cSimulation *simulationPtr;
        cEvent *event;
        cStaticFlag dummy;
        SectionBasedConfiguration *bootconfigptr;
        InifileReader *inifilePtr;
        bool needsCleaning = false;

        GymApi();

        void initialise(std::string inipath);
        std::unordered_map<std::string, ObsType > reset();
        // std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string, bool > > step(ActionType action);
        std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string, bool >, std::unordered_map<std::string,bool > > step(std::unordered_map<std::string, ActionType > actions);

        void cleanupmemory();

        void shutdown();
};

#endif
