#include <GymApi.h>
#include <omnetpp.h>
#include <unordered_map>
#include <stdlib.h>
#include <string.h>


using namespace std;
using namespace omnetpp;

int main(int argc, char **argv){

    std::string HOME(getenv("HOME"));
    std::string NEDPATH = "NEDPATH="+HOME + "/raynet/simulations;"+HOME+"/raynet/simlibs/RLComponents/src;"+HOME+"/raynet/simlibs/ecmp/src;"+HOME+"/raynet/simlibs/TcpPaced/src;"+HOME+"/raynet/simlibs/RLCC/src;"+HOME+"/raynet/simlibs/rdp/src;"+HOME+"/inet4.5/src/inet;"+HOME+"/inet4.5/examples";

    putenv(NEDPATH.c_str());
    // TODO: Initialise CmdRllibenv. This class will be bound to Python.
    std::cout << NEDPATH << std::endl;
    std::string _iniPath;
    ObsType  obs;

    _iniPath = (string(getenv("HOME"))+string("/raynet/configs/orca/orcaConfigStatic_debug.ini")).c_str();

    GymApi* gymapi = new GymApi();
   
   
   
    gymapi->initialise(_iniPath);
    auto id_obs = gymapi->reset();

    std::vector<std::string> keys;
    keys.reserve(id_obs.size());

    std::vector<ObsType> vals;
    vals.reserve(id_obs.size());

    for(auto kv : id_obs) {
        keys.push_back(kv.first);
        vals.push_back(kv.second);  
    } 

    std::string agentId = keys.front();

    bool done = false;
    bool simDone = false;
    while (!done && strcmp(agentId.c_str(), "SIMULATION_END") != 0 && !simDone) {
        for(std::unordered_map<std::string,ObsType>::iterator it = id_obs.begin(); it != id_obs.end(); ++it) {
            agentId = it->first;
            }
        std::unordered_map<std::string, ActionType> actions({ {agentId, 1} });
        auto ret = gymapi->step(actions);
        done = std::get<2>(ret)["__all__"];
        obs = std::get<0>(ret)[agentId];
        simDone = std::get<3>(ret)["simDone"];
    }

    gymapi->shutdown();
    gymapi->cleanupmemory();

    // gymapi->initialise(_iniPath);
    // id_obs = gymapi->reset();

    // keys.reserve(id_obs.size());
    // vals.reserve(id_obs.size());

    // for(auto kv : id_obs) {
    //     keys.push_back(kv.first);
    //     vals.push_back(kv.second);  
    // } 

    //  agentId = keys.front();

    // done = false;
    // while (!done && strcmp(agentId.c_str(), "nostep") != 0) {
    //     for(std::unordered_map<std::string,ObsType>::iterator it = id_obs.begin(); it != id_obs.end(); ++it) {
    //         agentId = it->first;
    //         }

    //     std::cout << "Agent Id is:" << agentId << std::endl;
    //     std::cout << "Agent ID printed" << std::endl;
    //     std::unordered_map<std::string, ActionType> actions({ {agentId, 0} });
    //     auto ret = gymapi->step(actions);
    //     done = std::get<2>(ret)["__all__"];
    //     obs = std::get<0>(ret)[agentId];
    // }

    // gymapi->shutdown();
    // gymapi->cleanupmemory();
    

    return 0;
}
