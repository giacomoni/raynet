//
// Copyright (C) 2020 Luca Giacomoni and George Parisis
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef INET_LEARNING_RLINTERFACE_RLINTERFACE_H_
#define INET_LEARNING_RLINTERFACE_RLINTERFACE_H_

#include <omnetpp.h>
#include "BrokerData.h"
#include <iostream>


#include "rlUtil.h"
#include "cobjects.h"
#include "typedefs.h"
#include <cmath>

using namespace omnetpp;

namespace learning {

class RLInterface : virtual public cListener
{
protected:
    // ------------------------------ LEGACY CODE -------------------------------
    // signal that carries stepData
    simsignal_t dataSignal;
    // signal that carries an action query
    simsignal_t querySignal;
    // variables to keep track of the transitioning of states and the action taken
    State rlOldState, rlState;
    int stateSize;
    bool isValid = true;
    ActionType lastMiAction;
    double delayWeightReward;
    // ---------------------------------------------------------------------------


    // string is used to identify the interface within this simulation
    std::string stringId;

    // the closest parent cSimpleModule used to emit signals (and have access to cSimpleModule methods)
    cComponent *owner;

    simsignal_t senderToStepper; // communication signal from sender to stepper.
    simsignal_t registerSig;
    simsignal_t unregisterSig;
    simsignal_t modifyStepSizeSig;

    bool rlInitialised = false;
    bool done; //Whether this agent's episode terminated
    bool isReset; //Whether the environemt is being reset right now

public:
// ------------------------------------------- LEGACY CODE -------------------------------------------------
    void setMaxObservationCount(int size); // sets the max number of observations that make a state
    void updateState(Observation obs); //updates the state by adding the observation obs
    float computeReward(float delta, float delay, float throughput); // computes reward
    int getStateSize() const;
    void setStateSize(int stateSize);
    void initialize(int stateSize, int obsMaxSize);
// -------------------------------------------------------------------------------------------------------

    ~RLInterface();
    void setOwner(cComponent *_owner); // sets the owner pointer. must be called in the initialize method
    void setStringId(std::string _id);

    void initialise();
    void terminate();

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override; // callback for receiving action response
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, const char *value, cObject *details) override; // callback for receiving action response

    // ------------ VIRTUAL FUNCTIONS -------------------
    virtual void cleanup() = 0;
    virtual void decisionMade(ActionType action) = 0; // defines what to do when decision is made
    virtual ObsType  getRLState() = 0;
    virtual RewardType getReward() = 0;
    virtual bool getDone() = 0;
    virtual void resetStepVariables() = 0;
    virtual ObsType computeObservation() = 0;
    virtual RewardType computeReward() = 0;
    // --------------------------------------------------

    tuple<array<double, 4>, int, int, string> getStepReturns(); //returns a tuple containg the obs, reward and done at the end of a step(MI)





    

};

} //namespace learning

#endif /* INET_LEARNING_RLINTERFACE_RLINTERFACE_H_ */
