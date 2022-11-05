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

#ifndef __STEPPER_H_
#define __STEPPER_H_

#include <omnetpp.h>
#include <iostream>
#include <string>
#include <math.h>
#include <array>
#include <random>
#include <tuple>
#include "BrokerData.h"
#include "cobjects.h"
#include <unordered_map>
#include "typedefs.h"

using namespace omnetpp;
using namespace std;


struct StepDetails{
  float stepSize; //Time in seconds 
  cMessage* stepMsg; // Msg used to notify end of step
  std::string rlId; // String ID returned into the dictionary of pthon step() function
  bool isReset; // Whether the next step is used as a RESET step, rather than normal step
};


class Stepper : public cSimpleModule, virtual public cListener
{
protected:
  // Map of active agents. Key is the string name of the agent (TODO: full path or just conn-N?), valiue is 
  // is a struct containing info on the current stepping logic
  std::unordered_map<std::string, StepDetails> activeAgents;

  // Map for easy retrieval of network component name given the rlId
  std::unordered_map<std::string, StepDetails> rlId2id;

  simsignal_t actionResponse = registerSignal("actionResponse"); //communication signal from stepper to sender with the action from ray
  simsignal_t pullObservations = registerSignal("pullObservations"); //communication signal from stepper to sender requestiing the obsevations.
  simsignal_t stepperToBroker = registerSignal("stepperToBroker"); //communication signal from stepper to broker with observations

  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
  void receiveSignal(cComponent *source, simsignal_t signalID, cObject *value, cObject *obj);
  void receiveSignal(cComponent *source, simsignal_t signalID, const char *value, cObject *obj);
  void receiveSignal(cComponent *src, simsignal_t id, double value, cObject *details);
public:
~Stepper();
};

#endif