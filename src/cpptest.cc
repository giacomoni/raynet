#include <SimulationRunner.h>
#include <omnetpp.h>
#include <unordered_map>

using namespace std;
using namespace omnetpp;

int main(int argc, char **argv){
    // TODO: Initialise CmdRllibenv. This class will be bound to Python.

    std::string _iniPath;

    _iniPath = (string(getenv("HOME"))+string("/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_template_debug.ini")).c_str();

    SimulationRunner* runner = new SimulationRunner();
    runner->initialise(_iniPath);
    auto obs = runner->reset();

    std::string agentId;

    bool done = false;
    while (!done) {
        for(std::unordered_map<std::string,ObsType>::iterator it = obs.begin(); it != obs.end(); ++it) {
            agentId = it->first;
            }

    
        std::unordered_map<std::string, ActionType> actions({ {agentId, 0} });
        auto ret = runner->step(actions);
        done = std::get<2>(ret)["__all__"];
        obs = std::get<0>(ret);
    }

    runner->shutdown();
    runner->cleanupmemory();
    
    // runner->initialise(_iniPath);
    // obs = runner->reset();

    // done = false;
    // while (!done) {
    //     for(std::unordered_map<std::string,ObsType>::iterator it = obs.begin(); it != obs.end(); ++it) {
    //         agentId = it->first;
    //         }

    
    //     std::unordered_map<std::string, ActionType> actions({ {agentId, 0} });
    //     auto ret = runner->step(actions);
    //     done = std::get<2>(ret)["__all__"];
    //     obs = std::get<0>(ret);
    // }

    // runner->shutdown();
    // runner->cleanupmemory();

    // delete runner;

    return 0;
}
