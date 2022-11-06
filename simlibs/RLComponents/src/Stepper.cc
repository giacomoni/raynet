#include "Stepper.h"

Define_Module(Stepper);

void Stepper::initialize()
{
 
   getSimulation()->getSystemModule()->subscribe("brokerToStepper", this); //suscribe to the broker's signal
   getSimulation()->getSystemModule()->subscribe("senderToStepper", this); //suscribe to the broker's signal
   getSimulation()->getSystemModule()->subscribe("registerAgent", this); // used to register stepping agents
   getSimulation()->getSystemModule()->subscribe("unregisterAgent", this);// used to unregister stepping agents
   getSimulation()->getSystemModule()->subscribe("modifyStepSize", this);// used to modify the size of the step used by Stepper


//TODO: unsubscribe
}

Stepper::~Stepper(){
    for (auto& it: activeAgents) {
        // Get agent id
        if (it.second.stepMsg->isScheduled()){
            cancelEvent(it.second.stepMsg);
            take(it.second.stepMsg);
        }
        delete it.second.stepMsg;
     }
}

/*
Only message is the end of step message. Pull the observation (actually RLStae) when this happens
*/
void Stepper::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()){
    EV_DEBUG << "MI period ended for " << msg->getName() << " at " << simTime() << endl;

    std::string delimiter = "-";
    std::string s(msg->getName());
    std::string id = s.substr(s.find(delimiter)+1);

    EV_TRACE << "Next MI ending period scheduled for simultaion time: " << simTime() + activeAgents[id].stepSize << endl;

    // Pass the agent's identifier for the observation we need.
    emit(pullObservations, id.c_str()); 
    }
    else{
        EV_DEBUG << "Stepper should only receive self messages!" << std::endl;
    }
}


void Stepper::receiveSignal(cComponent *source, simsignal_t signalID, const char *value, cObject *obj){
  const char *signalName = getSignalName(signalID);
  Enter_Method("schedule a step event(self message)");

  if (strcmp(signalName, "registerAgent") == 0){
        EV_TRACE << "Registering new agent with Stepper..." << std::endl;
        
        //Get id
        std::string id(value);

        EV_TRACE << "Agent ID: " << id << std::endl;

        cSimTime * stepSize = (cSimTime *) obj;

        // Creating detailsfor new agent
        StepDetails details;
        details.isReset = true;
        details.stepMsg = new cMessage((std::string("STEP-") + id).c_str()); // Name of the message should match STEP-<ID>
        details.stepSize = stepSize->simtime.dbl();
        details.rlId = id;

        delete stepSize;

        //Inserting new agent details into map
        activeAgents.insert({id,details});

        //Schedule first step
        scheduleAt(simTime() + details.stepSize, details.stepMsg);
        EV_TRACE << "Agent " << id << " will step in " <<  details.stepSize << " seconds at " << simTime() + details.stepSize << std::endl;

    }
    else if  (strcmp(signalName, "unregisterAgent") == 0){

        //Get id
        std::string id(value);

        EV_TRACE << "Removing agent from Stepper map: " << id << std::endl;

        // Remove agent details from map after deleting the msg
        if (activeAgents[id].stepMsg->isScheduled()){
            cancelEvent(activeAgents[id].stepMsg);
            take(activeAgents[id].stepMsg);
        }
        delete activeAgents[id].stepMsg;

        activeAgents.erase(id);
    }

}

/*
TODO: Rework comment
This signal handler method deals with 2 signals.
The first singal is from the Broker with the cwnd action. This will be passed on to RLInterface.
The second signal is from RLInterface with the state for the step. Triggered by stepper self message being recieved(MI period ended)
*/
void Stepper::receiveSignal(cComponent *source, simsignal_t signalID, cObject *value, cObject *obj)
{

    const char *signalName = getSignalName(signalID);

    
    // if signal is from the broker, it contains whether the MI is a reset or step(action). Pass it to RLInterface.
    if (strcmp(signalName, "brokerToStepper") == 0)
    {
        emit(actionResponse, value, obj); // Also pass the agent name (obj)
    }

    // else if signal is from RLInterface with the state. Pass it to broker
    else if (strcmp(signalName, "senderToStepper") == 0)
    {
        //Get id
        cString *c_id = (cString *) obj;
        std::string id = c_id->str;

        BrokerData *data = (BrokerData *)value;
        EV_TRACE << "Received signal senderToStepper from "<< id << std::endl;

        if (!data->getDone()){
            cancelEvent(activeAgents[id].stepMsg);
            take(activeAgents[id].stepMsg);
            scheduleAt(simTime() + activeAgents[id].stepSize, activeAgents[id].stepMsg);
        }

        EV_TRACE << "Sending data to broker..." << std::endl;
        emit(stepperToBroker, data, obj);

    }
    
    else
    {
        EV_ERROR << "Unknown signal " << signalName << ". Expecting either brokerToStepper signal or senderToStepper signal."<< endl;

    }
}

void Stepper::receiveSignal(cComponent *src, simsignal_t signalID, double value, cObject *obj){
    Enter_Method("change value of step size");

    const char *signalName = getSignalName(signalID);

     if (strcmp(signalName, "modifyStepSize") == 0){
        //Get id
        cString *c_id = (cString *) obj;
        std::string id = c_id->str;
        activeAgents[id].stepSize = float(value);

        if(activeAgents[id].stepMsg->isScheduled()){
            cancelEvent(activeAgents[id].stepMsg);
            take(activeAgents[id].stepMsg);
            scheduleAt(simTime() + activeAgents[id].stepSize, activeAgents[id].stepMsg);

        }

        delete obj;

    }

}
