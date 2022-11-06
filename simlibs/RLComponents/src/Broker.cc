/*
 * Broker.cpp
 *
 *  Created on: Sep 29, 2021
 *      Author: basil
 */

//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include "Broker.h"
#include <tuple>

// The module class needs to be registered with OMNeT++
Define_Module(Broker);

void Broker::finish(){
     for (auto& it: activeAgents) {
        // Get agent id
        if (it.second.endOfStep->isScheduled()){
            cancelEvent(it.second.endOfStep);
        }
        delete it.second.endOfStep;
     }
}

/*
Initialise the Broker by suscribing to the Sender signal
*/
void Broker::initialize()
{

    brokerToStepper = registerSignal("brokerToStepper");     // Comunication channel from Broker to Stepper

    getSimulation()->getSystemModule()->subscribe("stepperToBroker", this);  //Communication channel from Stepper to Broker;
    getSimulation()->getSystemModule()->subscribe("registerAgent", this); // used to register stepping agents
    getSimulation()->getSystemModule()->subscribe("unregisterAgent", this);// used to unregister stepping agents
   
    allAgentsDone = false;
}

/*
Recieved a self message from this module and do nothing.
This signifies an end of step. Do nothing to return and leave the simulation.
*/
void Broker::handleMessage(cMessage *msg)
{
    return;
}

void Broker::receiveSignal(cComponent *source, simsignal_t signalID, const char *value, cObject *obj){
    Enter_Method("schedule a step event(self message)"); //need this for direct messaging. Allows us to call scheduleMessage from the sender(ownership).
    const char *signalName = getSignalName(signalID);

    if (strcmp(signalName, "registerAgent") == 0){
        EV_TRACE << "Registering new agent with Broker..." << std::endl;
        
        std::string id(value);

        EV_TRACE << "Agent ID: " << id << std::endl;

        // Creating detailsfor new agent
        BrokerDetails details;
        details.isReset = true;
        details.endOfStep = new cMessage((std::string("EOS-") + id).c_str()); // Name of the message should match EOS-<ID>
        details.rlId = id;

        //Inserting new agent details into map
        activeAgents.insert({id,details});

    }
    else if (strcmp(signalName, "unregisterAgent") == 0){
        //Get id
        std::string id(value);

        //Set done for agent to true and step right away
        activeAgents[id].done = true;
        
        if(activeAgents[id].endOfStep->isScheduled())
            cancelEvent(activeAgents[id].endOfStep);

        // Schedule an EOS message for the specific agent.
        scheduleAt(simTime(), activeAgents[id].endOfStep);

        //Check if all agents are done and store in variable for SimulationRunner
        allAgentsDone = areAllAgentsDone();
    }


}

bool Broker::areAllAgentsDone(){

    bool allDone = true;
    for (auto& it: activeAgents) {
        if(!it.second.done)
            allDone = false;
     }
    return allDone;
}


/*
Store the state recieved from the sender module and schedule a self message("EnfOfStep") immediately to show an end of step event.
This is done immediately to return the state, and get the new action so that it can be used at the start of the next step(MI) allready scheduled
by the Stepper module. 
*/
void Broker::receiveSignal(cComponent *source, simsignal_t signalID, cObject *value, cObject *obj)
{
    Enter_Method("schedule a step event(self message)"); //need this for direct messaging. Allows us to call scheduleMessage from the sender(ownership).
    const char *signalName = getSignalName(signalID);

    
    if (strcmp(signalName, "stepperToBroker") == 0){
        BrokerData *data = (BrokerData *)value;

        //Get id
        cString *c_id = (cString *) obj;
        std::string id = c_id->str;

        activeAgents[id].observation = data->getObs();

        if (!data->isReset()){
            activeAgents[id].reward = data->getReward();
            activeAgents[id].done = data->getDone();
        }

        
        if(activeAgents[id].endOfStep->isScheduled())
            cancelEvent(activeAgents[id].endOfStep);

        // Schedule an EOS message for the specific agent.
        scheduleAt(simTime(), activeAgents[id].endOfStep);




        EV_TRACE <<  simTime() <<" Scheduled end of step for " << id << "..." << std::endl;

        //Clean up
        delete obj;
        delete value;
    }
    else{
        EV_TRACE << "Signal received by Broker not recognised" << std::endl;
    }

}



void Broker::setActionAndMove(std::unordered_map<std::string, std::tuple<ActionType, bool>> &actionsAndMoves)
{
    BrokerData *data;
    cString *obj;
    // Set action and move for each agent in the unordered map
    for (auto& it: actionsAndMoves) {
        data = new BrokerData();

        if (std::get<1>(it.second)){
            data->setReset(std::get<1>(it.second));
            }

        else //move is action step
        {
            data->setAction(std::get<0>(it.second));
            data->setReset(false);
        }

        obj = new cString(it.first);
        emit(brokerToStepper, data, obj);
        delete data;
        delete obj;
    }


}

ObsType Broker::getObservation(std::string id){

    return activeAgents[id].observation;

}

std::unordered_map<std::string, ObsType> Broker::getObservations(){

    std::unordered_map<std::string, ObsType> observations;
     for (auto& it: activeAgents) {
        // Get agent id
        std::string id = it.first;
        auto observation = it.second.observation;
        observations.insert({id, observation});
     }

     return observations;
}

  // Get the reward
RewardType Broker::getReward(std::string id){
    return activeAgents[id].reward;

}

std::unordered_map<std::string, RewardType> Broker::getRewards(){

      std::unordered_map<std::string,RewardType> rewards;
     for (auto& it: activeAgents) {
        // Get agent id
        std::string id = it.first;
        auto reward = it.second.reward;
        rewards.insert({id, reward});
     }

     return rewards;

}

bool Broker::getDone(std::string id)
{
    return activeAgents[id].done;
}

std::unordered_map<std::string, bool> Broker::getDones(){
    std::unordered_map<std::string, bool> dones;
     for (auto& it: activeAgents) {
        // Get agent id
        std::string id = it.first;
        auto done = it.second.done;
        dones.insert({id, done});
     }

     return dones;
}

bool Broker::getAllDone(){
    return allAgentsDone;
}
