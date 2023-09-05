

#ifdef CARTPOLE
#include "CartpoleComponent.h"


Define_Module(CartpoleComponent);

CartpoleComponent::~CartpoleComponent(){
    cancelAndDelete(initMsg);
}

void CartpoleComponent::initialize()
{
    steps=0;	
    gravity = 9.8;
    masscart = 1.0;
    masspole = 0.1;
    total_mass = masspole + masscart;
    length = 0.5; // actually half the pole's length
    polemass_length = masspole * length;
    force_mag = 10.0;
    tau = 0.02; // seconds between state updates
    kinematics_integrator = "euler";

    // Angle at which to fail the episode
    theta_threshold_radians = 12 * 2 * M_PI / 360;
    x_threshold = 2.4;

    // Angle limit set to 2 * theta_threshold_radians so failing observation
    // is still within bounds.
    // high[0] = x_threshold * 2;
    // high[1] = 3.4028235e+38;
    // high[2] = theta_threshold_radians * 2;
    // high[3] = 3.4028235e+38;

    high[0] = 3.4028235e+38;
    high[1] = 3.4028235e+38;
    high[2] = 3.4028235e+38;
    high[3] = 3.4028235e+38;

    steps_beyond_done = -10;
    a = 2;
    b = 13;    

    state = random();

    isRegistered = false;

    initMsg = new cMessage("CARTPOLE-INIT"); 
    scheduleAt(simTime() + 1, initMsg);
}
void CartpoleComponent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()){
    scheduleAt(simTime() + 1, initMsg);
    
    if(!isRegistered){
        isRegistered = true;
        cObject* simtime = new cSimTime(1);
        this->setOwner(this);
        RLInterface::initialise();

        // Generate ID for this agent
        std::string s("cartpole");
        this->setStringId(s);



        emit(this->registerSig, stringId.c_str(), simtime);
    }
    }
    else{
        EV_DEBUG << "Stepper should only receive self messages!" << std::endl;
    }
}

ObsType CartpoleComponent::random()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-0.05, 0.05);
    ObsType values;
    for (int n = 0; n < 50; ++n)
        values[n] = dis(gen);
    return values;
}

void CartpoleComponent::step(ActionType action)
{
    double x = state[0];
    double x_dot = state[1];
    double theta = state[2];
    double theta_dot = state[3];
    double force;
    steps++;
    if (action == 1)
    {
        force = force_mag;
    }
    else
    {
        force = force_mag * -1;
    }

    double costheta = cos(theta);
    double sintheta = sin(theta);

    double temp = (force + polemass_length * pow(theta_dot, 2) * sintheta) / total_mass;

    double thetaacc = (gravity * sintheta - costheta * temp) / (length * (4.0 / 3.0 - masspole * pow(costheta, 2) / total_mass));

    double xacc = temp - polemass_length * thetaacc * costheta / total_mass;

    // cout << "temp: " << temp << " thetaacc: " << thetaacc << " xacc: " << xacc <<endl;

    if (kinematics_integrator == "euler")
    {
        x = x + tau * x_dot;
        x_dot = x_dot + tau * xacc;
        theta = theta + tau * theta_dot;
        theta_dot = theta_dot + tau * thetaacc;
    }
    else
    {
        x_dot = x_dot + tau * xacc;
        x = x + tau * x_dot;
        theta_dot = theta_dot + tau * thetaacc;
        theta = theta + tau * theta_dot;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.05, 0.05);

    for (int n = 0; n < 50; ++n) {
        state[n]=dist(gen);
    }
    // state = {x, x_dot, theta, theta_dot};

}

void CartpoleComponent::cleanup(){}

void  CartpoleComponent::decisionMade(ActionType action){
    if(this->isReset){
        //Reset state for next iterations
        steps_beyond_done = -10;
        state = random();
        std::cout << "Environment reset!" << std::endl;
    }else{
        step(action);
    }

} // defines what to do when decision is made
ObsType CartpoleComponent::getRLState(){
    return state;
}

RewardType CartpoleComponent::getReward(){
    RewardType reward;
    if (done == false)
    {
        reward = 1.0;
    }
    else if (steps_beyond_done == -10)
    {
        // Pole just fell
        steps_beyond_done = 0;
        reward = 1.0;
    }
    else
    {
        if (steps_beyond_done == 0)
        {
            cout << "logger warn. step beyond done. reset should be called" << endl;
        }
        steps_beyond_done += 1;
        reward = 0.0;
    }

    return reward;
}
bool CartpoleComponent::getDone(){
    bool done = false;

    if (steps>= 500 || state[0] < x_threshold * -1 || state[0] > x_threshold || state[2] < theta_threshold_radians * -1 || state[2] > theta_threshold_radians)
    {
        done = true;
    }

    return done;

}
void CartpoleComponent::resetStepVariables(){

}
ObsType CartpoleComponent::computeObservation(){
    return getRLState();

}
RewardType CartpoleComponent::computeReward(){
    return getReward();
}

void CartpoleComponent::finish(){

}

// void CartpoleComponent::receiveSignal(cComponent *source, simsignal_t signalID, cObject *value, cObject *obj)
// {

//     BrokerData *data = (BrokerData *)value;
//     string b = data->getMove();

//     string reset_string("reset");
//     if (b.compare(reset_string) == 0)
//     {

//         array<double, 4> resetState = cpe->reset();

//         BrokerData *return_data = new BrokerData();
//         return_data->setMove("reset");
//         return_data->setObs(resetState);

//         emit(senderToBroker, return_data);
//         return;
//     }

//     else if (b.compare("action") == 0)
//     {
//         int act = data->getAction();
//         tuple<array<double, 4>, int, int> stepReturns = cpe->step(act);
//         array<double, 4> stepObs = get<0>(stepReturns);
//         int reward = get<1>(stepReturns);
//         int done = get<2>(stepReturns);

//         BrokerData *return_data = new BrokerData();
//         return_data->setMove("action");
//         return_data->setObs(stepObs);
//         return_data->setDone(done);
//         return_data->setReward(reward);

//         emit(senderToBroker, return_data);
//         return;
//     }
// }
#endif
