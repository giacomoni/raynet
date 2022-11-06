#include <GymApi.h>
#include <omnetpp.h>
#include <unordered_map>

using namespace std;
using namespace omnetpp;

int main(int argc, char **argv){
    // TODO: Initialise CmdRllibenv. This class will be bound to Python.

    std::string _iniPath;

    _iniPath = (string(getenv("HOME"))+string("/RLlibIntegration/configs/ndpconfig_single_flow_eval_with_delay_template_debug.ini")).c_str();

    GymApi* gymapi = new GymApi();
    gymapi->initialise(_iniPath);
    auto obs = gymapi->reset();

    std::string agentId;

    bool done = false;
    while (!done) {
        for(std::unordered_map<std::string,ObsType>::iterator it = obs.begin(); it != obs.end(); ++it) {
            agentId = it->first;
            }

    
        std::unordered_map<std::string, ActionType> actions({ {agentId, 0} });
        auto ret = gymapi->step(actions);
        done = std::get<2>(ret)["__all__"];
        obs = std::get<0>(ret);
    }

    gymapi->shutdown();
    gymapi->cleanupmemory();
    
    // gymapi->initialise(_iniPath);
    // obs = gymapi->reset();

    // done = false;
    // while (!done) {
    //     for(std::unordered_map<std::string,ObsType>::iterator it = obs.begin(); it != obs.end(); ++it) {
    //         agentId = it->first;
    //         }

    
    //     std::unordered_map<std::string, ActionType> actions({ {agentId, 0} });
    //     auto ret = gymapi->step(actions);
    //     done = std::get<2>(ret)["__all__"];
    //     obs = std::get<0>(ret);
    // }

    // gymapi->shutdown();
    // gymapi->cleanupmemory();

    // delete gymapi;

    return 0;
}
