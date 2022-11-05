#include <GymApi.h>
#include <string>


GymApi::GymApi(){   
}

void GymApi::cleanupmemory(){

    getSimulation()->deleteNetwork();
    cSimulation::setActiveSimulation(nullptr);
    delete simulationPtr; // deletes env as well
    // CodeFragments::executeAll(CodeFragments::SHUTDOWN);

    // needsCleaning = false;
    // //delete env;
    
    // cSimulation::setActiveSimulation(nullptr);
    // delete simulationPtr;
    // delete event;
    // delete bootconfigptr;
    // delete inifilePtr;
    
}

void GymApi::initialise(std::string _iniPath){
    needsCleaning = true;
    // initializations
    CodeFragments::executeAll(CodeFragments::STARTUP);
    SimTime::setScaleExp(-12);
    
    // char s1[] = "";
    std::vector<char*> cstrings; // final arguments for Omnet++

    // set up an environment for the simulation
    env = new Cmdrlenv();
    
    bootconfigptr = new SectionBasedConfiguration();
    inifilePtr = new InifileReader(); 

    //Read simulation configuration parameters from inifile
    inifilePtr->readFile(_iniPath.c_str());

    // activate [General] section so that we can read global settings from it
    bootconfigptr->setConfigurationReader(inifilePtr);

    simulationPtr = new cSimulation("simulation", env);
    cSimulation::setActiveSimulation(simulationPtr);
    
    env->initialiseEnvironment(cstrings.size(), &cstrings[0],bootconfigptr);

}


 std::unordered_map<std::string, ObsType > GymApi::reset(){
    // Reset the environment
    bool isReset = true;

    std::unordered_map<std::string, ObsType > resetObs;
    string networkname("simplenetwork");

    // run the simulation

    std::string id = env->step(0, isReset, networkname);

    if(id != "nostep"){
        cModule *mod = getSimulation()->getModuleByPath((networkname+string(".broker")).c_str());
        Broker *target = check_and_cast<Broker *>(mod);


        auto obss = target->getObservations();

        auto it = obss.begin();

        while (it != obss.end()) {
            // Check if key's first character is F
            if (it->first != id) {
                // erase() function returns the iterator of the next
                // to last deleted element.
                it = obss.erase(it);
            } else
                it++;
        }
        return obss;
    }
    else{
        ObsType obs;
        std::unordered_map<std::string, ObsType> obss = { {"nostep", obs} };
        return obss;
    }
    
}

// std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string,bool > > GymApi::step(ActionType action){
    
//     std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string,bool > > returnTuple;
//     bool isReset = false;

//     string networkname("simplenetwork");
//     // We call step on the environment
//     env->step(action, isReset, networkname);


//     cModule *mod = getSimulation()->getModuleByPath((networkname+string(".broker")).c_str());
//     Broker *target = check_and_cast<Broker *>(mod);
    
//     returnTuple = {target->getObservations(), target->getRewards(), target->getDones()};

//     return returnTuple;
// }

std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string,bool > > GymApi::step(std::unordered_map<std::string, ActionType> actions){
    
    std::tuple<std::unordered_map<std::string, ObsType >, std::unordered_map<std::string, RewardType > , std::unordered_map<std::string,bool > > returnTuple;
    bool isReset = false;

    string networkname("simplenetwork");
    // We call step on the environment
    std::string id = env->step(actions, isReset, networkname);

    cModule *mod = getSimulation()->getModuleByPath((networkname+string(".broker")).c_str());
    Broker *target = check_and_cast<Broker *>(mod);

    auto obss = target->getObservations();
    auto rewards = target->getRewards();
    auto dones = target->getDones();
    bool allDone = target->getAllDone();
    
    auto obss_it = obss.begin();
    while (obss_it != obss.end()) {
    // Check if key's first character is F
        if (obss_it->first != id) {
            // erase() function returns the iterator of the next
            // to last deleted element.
            obss_it = obss.erase(obss_it);
        } else
            obss_it++;
    }

    auto rewards_it = rewards.begin();
     while (rewards_it != rewards.end()) {
        // Check if key's first character is F
        if (rewards_it->first != id) {
            // erase() function returns the iterator of the next
            // to last deleted element.
            rewards_it = rewards.erase(rewards_it);
        } else
            rewards_it++;
    }

    auto dones_it = dones.begin();
        
    while (dones_it != dones.end()) {
        // Check if key's first character is F
        if (dones_it->first != id) {
            // erase() function returns the iterator of the next
            // to last deleted element.
            dones_it = dones.erase(dones_it);
        } else
            dones_it++;
    }


    dones.insert({"__all__", allDone});

    returnTuple = { obss, rewards, dones };

    return returnTuple;
}


void GymApi::shutdown(){
    env->endSimulation();
}
  
