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
#include "RLInterface.h"

namespace learning {
// --------------------------- LEGACY CODE -------------------------------
void RLInterface::setMaxObservationCount(int size)
{
    rlOldState.maxObservationsCount = size;
    rlState.maxObservationsCount = size;
}
void RLInterface::updateState(Observation obs)
{
    rlOldState = rlState;
    rlState.addObservation(obs);
}
int RLInterface::getStateSize() const
{
    return stateSize;
}

void RLInterface::setStateSize(int stateSize)
{
    this->stateSize = stateSize;
}

float RLInterface::computeReward(float delta, float delay, float throughput)
{
    if (isnan(delay) or isnan(throughput)) {
        throughput = 0;
        delay = 0;
        EV_WARN << "Delay or Throughput value is NaN during computation of the reward" << std::endl;
    }

    return throughput - delta * delay;
}


RLInterface::~RLInterface()
{}

void RLInterface::setStringId(std::string _id)
{
    stringId = _id;
}

void RLInterface::initialize(int stateSize, int maxObsSize)
{
    std::cerr << "Registering Interface" << std::endl;
    senderToStepper = owner -> registerSignal("senderToStepper"); 
    registerSig = owner->registerSignal("registerAgent");
    unregisterSig  = owner->registerSignal("unregisterAgent");
    modifyStepSizeSig = owner->registerSignal("modifyStepSize");
    
    getSimulation()->getSystemModule()->subscribe("actionResponse", (cListener*) this);

    done = false;
    isReset = false;
    lastMiAction = 0;
}

void RLInterface::initialise()
{
    senderToStepper = owner -> registerSignal("senderToStepper"); 
    registerSig = owner->registerSignal("registerAgent");
    unregisterSig  = owner->registerSignal("unregisterAgent");
    modifyStepSizeSig = owner->registerSignal("modifyStepSize");

    getSimulation()->getSystemModule()->subscribe("actionResponse", (cListener*) this);
    getSimulation()->getSystemModule()->subscribe("pullObservations", (cListener*) this);
    

    done = false;
    isReset = false;
    lastMiAction = 0;
}



void RLInterface::setOwner(cComponent *_owner)
{
    owner = _owner;
}

// signal handler method for recieving the action from the agent.
// Also inform about the type of step - reset or step(action)
// If reset: This method will not be called as MyTCPAlgorithm has not been initialised yet. To overcome this initialised move = "reset"
void RLInterface::receiveSignal(cComponent *source, simsignal_t id, cObject *value, cObject *details)
{
    const char *signalName = owner->getSignalName(id);

    
    if (strcmp(signalName, "actionResponse") == 0)
    {
        cString * c_id = dynamic_cast< cString *>(details);
        std::string id = c_id->str;
        std::string cartpolestr("cartpole");
        std::string resetstr("RESET");

         if (strcmp(stringId.c_str(), cartpolestr.c_str()) == 0 && strcmp(id.c_str(), resetstr.c_str()) == 0){
            BrokerData *data = dynamic_cast< BrokerData *>(value);
            isReset = true;
            ActionType decision = data->getAction();

            decisionMade(decision);
        }
        // If this signal refers to this agent, then take the action.
        if (strcmp(stringId.c_str(), id.c_str()) == 0){
            BrokerData *data = dynamic_cast< BrokerData *>(value);

            if (!data->isReset()){
                ActionType decision = data->getAction();

                decisionMade(decision);
                // Reset the variables that keep track of step wise stats.
                resetStepVariables();
            }
            else{
                isReset = true;
            }

        }

       

    }
    else {
        EV_ERROR << "Unknown signal " << signalName << std::endl;
    }


}

// signal handler method for pull Observations request from Stepper. 
// This method signifies that the Stepper has reached the end of a step(MI) and the state is needed to compute the next action.
// Returning the state information back to stepper.
//todo: set the move in a data structure(stepReturns) and use this to compare
void RLInterface::receiveSignal(cComponent *source, simsignal_t id, const char * value, cObject *details)
{
    const char *signalName = owner->getSignalName(id);

    if (strcmp(signalName, "pullObservations") == 0)
    {
        if (strcmp(value, stringId.c_str()) == 0){
            BrokerData *return_data = new BrokerData();

            //We only care about the observation in a reset call
            return_data->setReset(isReset);
            //TODO: compute actual observastion
            return_data->setObs(computeObservation());
            if (!isReset){
                return_data->setDone(done);
                //TODO: compute actual reward
                return_data->setReward(computeReward());
            }

            // isReset should only be true once: at the start of the simulation.
            isReset = false;
            
            EV_TRACE << stringId << " is sending step data to stepper..." << std::endl;

            cString * obj = new cString(stringId);
            owner->emit(senderToStepper, return_data, obj); 
        }
    }
    else
    {
        EV_ERROR << "Unknown signal " << signalName << ". Expecting pullObservation signal." << std::endl;
    }
}

}