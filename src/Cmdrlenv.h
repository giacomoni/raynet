#ifndef __OMNETPP_CMDRLENV_CMDENV_H
#define __OMNETPP_CMDRLENV_CMDENV_H


#include <assert.h>
#include <omnetpp.h>
#include <cmdenv/cmddefs.h>
#include <cmdenv/cmdenv.h>
#include <envir/speedometer.h>
#include <common/stringutil.h>
#include <Broker.h>
#include <unordered_map>
#include <tuple>

using namespace omnetpp;
using namespace omnetpp::cmdenv;
using namespace omnetpp::envir;
using namespace omnetpp::common;
using namespace std;


/**
 * Command line user interface.
 */
class Cmdrlenv : public Cmdenv
{
  public:
    Cmdrlenv();
    // Set up the simulation
    void initialiseEnvironment(int argc, char *argv[],cConfiguration *configobject);
    // Execute events until the next step event
    std::string step(ActionType action, bool isReset);
    std::string step(std::unordered_map<std::string, ActionType>  actions, bool isReset);

    // Shutdown the simulation
    void endSimulation();

    double getReward();
    std::vector<double> getObservation();

};

#endif