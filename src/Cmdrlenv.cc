#include <Cmdrlenv.h>

Cmdrlenv::Cmdrlenv(){}


// note: also updates "since" (sets it to the current time) if answer is "true"
inline bool elapsed(long millis, int64_t& since)
{
    int64_t now = opp_get_monotonic_clock_usecs();
    bool ret = (now - since) > millis * 1000;
    if (ret)
        since = now;
    return ret;
}

void  Cmdrlenv::initialiseEnvironment(int argc, char *argv[],cConfiguration *configobject){
    opt = createOptions();
    args = new ArgList();
    args->parse(argc, argv, ARGSPEC);  // TODO share spec with startup.cc!
    opt->useStderr = !args->optionGiven('m');
    opt->verbose = !args->optionGiven('s');
    cfg = dynamic_cast<cConfigurationEx *>(configobject);
    if (!cfg)
        throw cRuntimeError("Cannot cast configuration object %s to cConfigurationEx", configobject->getClassName());

    if(setup()){
        // '-c' and '-r' option: configuration to activate, and run numbers to run.
        // Both command-line options take precedence over inifile settings.
        // (NOTE: inifile settings *already* got read at this point! as EnvirBase::setup()
        // invokes readOptions()).

        if (args->optionGiven('c'))  // note: do not overwrite value from cmdenv-config-name option
            opt->configName = args->optionValue('c');
        if (opt->configName.empty())
            opt->configName = "General";

        if (args->optionGiven('r'))  // note: do not overwrite value from cmdenv-runs-to-execute option!
            opt->runFilter = args->optionValue('r');

        std::vector<int> runNumbers;
        try {
            runNumbers = resolveRunFilter(opt->configName.c_str(), opt->runFilter.c_str());
        }
        catch (std::exception& e) {
            displayException(e);
            exitCode = 1;
            return;
        }

        bool finishedOK = false;
        bool networkSetupDone = false;
        bool endRunRequired = false;
        try{
            if (opt->verbose)
                    out << "\nPreparing for running configuration " << opt->configName << ", run #" << 0 << "..." << endl;

                cfg->activateConfig(opt->configName.c_str(), 0);
                readPerRunOptions();

                const char *iterVars = cfg->getVariable(CFGVAR_ITERATIONVARS);
                const char *runId = cfg->getVariable(CFGVAR_RUNID);
                const char *repetition = cfg->getVariable(CFGVAR_REPETITION);
                if (!opt->verbose)
                    out << opt->configName << " run " << 0 << ": " << iterVars << ", $repetition=" << repetition << endl; // print before redirection; useful as progress indication from opp_runall

                if (opt->redirectOutput) {
                    processFileName(opt->outputFile);
                    if (opt->verbose)
                        out << "Redirecting output to file \"" << opt->outputFile << "\"..." << endl;
                    startOutputRedirection(opt->outputFile.c_str());
                    if (opt->verbose)
                        out << "\nRunning configuration " << opt->configName << ", run #" << 0 << "..." << endl;
                }

                if (opt->verbose) {
                    if (iterVars && strlen(iterVars) > 0)
                        out << "Scenario: " << iterVars << ", $repetition=" << repetition << endl;
                    out << "Assigned runID=" << runId << endl;
                }

                // find network
                if (opt->networkName.empty())
                    throw cRuntimeError("No network specified (missing or empty network= configuration option)");
                cModuleType *network = resolveNetwork(opt->networkName.c_str());
                ASSERT(network);

                endRunRequired = true;

                // set up network
                if (opt->verbose)
                    out << "Setting up network \"" << opt->networkName.c_str() << "\"..." << endl;

                setupNetwork(network);
                networkSetupDone = true;

                // prepare for simulation run
                if (opt->verbose)
                    out << "Initializing..." << endl;

                loggingEnabled = !opt->expressMode;
                
                prepareForRun();

                // run the simulation
                if (opt->verbose)
                    out << "\nSimulation initilization completed.." << endl;
                // simulate() should only throw exception if error occurred and
                // finish() should not be called.
                notifyLifecycleListeners(LF_ON_SIMULATION_START);

                installSignalHandler();

                startClock();
        }catch (std::exception& e) {
                loggingEnabled = true;
                stoppedWithException(e);
                notifyLifecycleListeners(LF_ON_SIMULATION_ERROR);
                displayException(e);
            }

    }
}

double Cmdrlenv::getReward(){
    //Access the reward value in the PolicyBroker
    // cModule *module = getSimulation()->getModuleByPath("Network.absolute.path");
    // assert(module!=nullptr);
    // PolicyBroker *policyBroker = check_and_cast<PolicyBroker*>(module);
    // return policyBroker->getReward();
    return 1.0;
}

std::vector<double> Cmdrlenv::getObservation(){
    //Access the reward value in the PolicyBroker
    // cModule *module = getSimulation()->getModuleByPath("Network.absolute.path");
    // assert(module!=nullptr);
    // PolicyBroker *policyBroker = check_and_cast<PolicyBroker*>(module);
    // return policyBroker->getObservation();
    std::vector<double> obs{1.0, 2.0, 3.0};
    return obs;
}


std::string Cmdrlenv::step(ActionType action, bool isReset){
    sigintReceived = false;
    Speedometer speedometer;

    /*
    * Send signal to RLInterface informing it's either a reset or step move going to happen
    * If it's reset - this will help only return the observations.
    * If it's step - also send the agent action in the signal. 
    */

    std::string broker_name = getSimulation()->getSystemModule()->getFullPath() + std::string(".broker");

    cModule *mod = getSimulation()->getModuleByPath(broker_name.c_str());

    Broker *target = check_and_cast<Broker *>(mod);

    std::unordered_map<std::string, std::tuple<ActionType, bool>> actionAndMove({{"RESET",std::tuple<ActionType,bool>{action, isReset}}});

    target->setActionAndMove(actionAndMove);

    #define FINALLY() { \
        if (opt->expressMode) \
            doStatusUpdate(speedometer); \
        loggingEnabled = true; \
        stopClock(); \
        deinstallSignalHandler(); \
    }

    // only used by Express mode, but we need it in catch blocks too
    try {
        if (!opt->expressMode) {
            
            
            //TODO: set action in the PolicyBroker and resume simulation
            while (true) {
                
                cEvent *event = getSimulation()->takeNextEvent();

                if (!event)
                    throw cTerminationException("Scheduler interrupted while waiting");

                // flush *between* printing event banner and event processing, so that
                // if event processing crashes, it can be seen which event it was
                if (opt->autoflush)
                    out.flush();
                
              
                /*
                * Reached an End of step where the event executes the handle msg of Broker.
                * Exec
                * Get the state of the cartpole env from Broker module
                */
               
                string eventName = event -> getName();
                getSimulation()->executeEvent(event);                

                if (eventName.find(std::string("EOS")) != std::string::npos) {
                    std::string agentId = eventName.substr(eventName.find(std::string("-"))+1);
                    return agentId;
                    }
       
                // flush so that output from different modules don't get mixed
                cLogProxy::flushLastLine();

                checkTimeLimits();
                if (sigintReceived)
                    throw cTerminationException("SIGINT or SIGTERM received, exiting");

             

            }
        }
        else {
            speedometer.start(getSimulation()->getSimTime());

            int64_t last_update = opp_get_monotonic_clock_usecs();

            // doStatusUpdate(speedometer);

           
            while (true) {
                cEvent *event = getSimulation()->takeNextEvent();                
              
                if (!event)
                    throw cTerminationException("Scheduler interrupted while waiting");

                speedometer.addEvent(getSimulation()->getSimTime());  // XXX potential performance hog
                
                // print event banner from time to time
                if ((getSimulation()->getEventNumber()&0xff) == 0 && elapsed(opt->statusFrequencyMs, last_update))
                    doStatusUpdate(speedometer);


                 /*
                * Get the observations of the cartpole env from Broker module
                */
                string eventName = event -> getName();

                // execute event
                getSimulation()->executeEvent(event);
                if (eventName.find(std::string("EOS")) != std::string::npos) {
                    std::string agentId = eventName.substr(eventName.find(std::string("-"))+1);
                    return agentId;
                    }
                
                
                checkTimeLimits();  // XXX potential performance hog (maybe check every 256 events, unless "cmdenv-strict-limits" is on?)
                if (sigintReceived)
                    throw cTerminationException("SIGINT or SIGTERM received, exiting");

            }
        }
    }
    catch (cTerminationException& e) {
        FINALLY();

        stoppedWithTerminationException(e);
        displayException(e);
        return "nostep";
    }
    catch (std::exception& e) {
        FINALLY();
        throw;
    }
    // note: C++ lacks "finally": lines below need to be manually kept in sync with catch{...} blocks above!
    if (opt->expressMode)
        doStatusUpdate(speedometer);

#undef FINALLY
}

std::string Cmdrlenv::step(std::unordered_map<std::string, ActionType>  actions, bool isReset){
    sigintReceived = false;
    Speedometer speedometer;

    /*
    * Send signal to RLInterface informing it's either a reset or step move going to happen
    * If it's reset - this will help only return the observations.
    * If it's step - also send the agent action in the signal. 
    */

    std::string broker_name = getSimulation()->getSystemModule()->getFullPath() + std::string(".broker");

    cModule *mod = getSimulation()->getModuleByPath(broker_name.c_str());

    Broker *target = check_and_cast<Broker *>(mod);

    std::unordered_map<std::string, std::tuple<ActionType, bool>> actionAndMove;

    for (auto& it: actions) {
        // Do stuff
        actionAndMove.insert({it.first, std::tuple<ActionType,bool>{it.second, isReset}});
}

    target->setActionAndMove(actionAndMove);

    // only used by Express mode, but we need it in catch blocks too
    try {
        if (!opt->expressMode) {
            
            
            //TODO: set action in the PolicyBroker and resume simulation
            while (true) {
                
                cEvent *event = getSimulation()->takeNextEvent();

                if (!event)
                    throw cTerminationException("Scheduler interrupted while waiting");

                // flush *between* printing event banner and event processing, so that
                // if event processing crashes, it can be seen which event it was
                if (opt->autoflush)
                    out.flush();
                
              
                /*
                * Reached an End of step where the event executes the handle msg of Broker.
                * Exec
                * Get the state of the cartpole env from Broker module
                */
                string eventName = event -> getName();
                getSimulation()->executeEvent(event);
                // std::cout << eventName << std::endl;

                if (eventName.find(std::string("EOS")) != std::string::npos) {
                    std::string agentId = eventName.substr(eventName.find(std::string("-"))+1);
                    return agentId;
                    }
       
                // flush so that output from different modules don't get mixed
                cLogProxy::flushLastLine();

                checkTimeLimits();
                if (sigintReceived)
                    throw cTerminationException("SIGINT or SIGTERM received, exiting");

             

            }
        }
        else {
            speedometer.start(getSimulation()->getSimTime());

            int64_t last_update = opp_get_monotonic_clock_usecs();

            // doStatusUpdate(speedometer);

           
            while (true) {
                cEvent *event = getSimulation()->takeNextEvent();
                // std::cout << eventName << std::endl;
                // execute event
              
                if (!event)
                    throw cTerminationException("Scheduler interrupted while waiting");

                speedometer.addEvent(getSimulation()->getSimTime());  // XXX potential performance hog
                
                // print event banner from time to time
                if ((getSimulation()->getEventNumber()&0xff) == 0 && elapsed(opt->statusFrequencyMs, last_update))
                    doStatusUpdate(speedometer);


                 /*
                * Get the observations of the cartpole env from Broker module
                */
               
                string eventName = event -> getName();
                getSimulation()->executeEvent(event);
                if (eventName.find(std::string("EOS")) != std::string::npos) {
                    std::string agentId = eventName.substr(eventName.find(std::string("-"))+1);
                    return agentId;
                    }
                
                
                checkTimeLimits();  // XXX potential performance hog (maybe check every 256 events, unless "cmdenv-strict-limits" is on?)
                if (sigintReceived)
                    throw cTerminationException("SIGINT or SIGTERM received, exiting");

               
                
               

            }
        }
    }
    catch (cTerminationException& e) {
        if (opt->expressMode)
            doStatusUpdate(speedometer);
        loggingEnabled = true;
        stopClock();
        deinstallSignalHandler();

        stoppedWithTerminationException(e);
        displayException(e);
        return;
    }
    catch (std::exception& e) {
        if (opt->expressMode)
            doStatusUpdate(speedometer);
        loggingEnabled = true;
        stopClock();
        deinstallSignalHandler();
        throw;
    }
    // note: C++ lacks "finally": lines below need to be manually kept in sync with catch{...} blocks above!
    if (opt->expressMode)
        doStatusUpdate(speedometer);
}


void Cmdrlenv::endSimulation(){
    loggingEnabled = true;

    if (opt->verbose)
        out << "\nCalling finish() at end of Run" << endl;
    getSimulation()->callFinish();
    cLogProxy::flushLastLine();

    checkFingerprint();

    notifyLifecycleListeners(LF_ON_SIMULATION_SUCCESS);

    shutdown();
}