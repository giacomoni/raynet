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
#ifdef CARTPOLE
#ifndef __CARTPOLE_COMPONENT_H_
#define __CARTPOLE_COMPONENT_H_

#include <omnetpp.h>
#include <iostream>
#include <string>
#include <math.h>
#include <array>
#include <random>
#include <tuple>
#include "BrokerData.h"
#include "RLInterface.h"



using namespace omnetpp;
using namespace std;
using namespace learning;

/**
 * TODO - Generated class
 */
class CartpoleComponent : public cSimpleModule, public RLInterface
{
public:
    double a;
    double b;
    double gravity;
    double masscart;
    double masspole;
    double total_mass;
    double length; // actually half the pole's length
    double polemass_length;
    double force_mag;
    double tau; // seconds between state updates
    string kinematics_integrator;

    double theta_threshold_radians;
    double x_threshold;

    double high[4];

    ActionType action[2] = {0, 1};

    int steps_beyond_done;
    int steps;
    ObsType state; // array declared

    cMessage* initMsg; // Msg used to notify end of step

    bool isRegistered;


protected:
  virtual void initialize();

public:
  ~CartpoleComponent();
  ObsType random();
  void step(ActionType action);
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    virtual void cleanup();
    virtual void decisionMade(ActionType action); // defines what to do when decision is made
    virtual ObsType  getRLState();
    virtual RewardType getReward();
    virtual bool getDone();
    virtual void resetStepVariables();
    virtual ObsType computeObservation();
    virtual RewardType computeReward();
  
};


#endif
#endif