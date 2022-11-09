#include <GymApi.h>
#include <omnetpp.h>
#include <unordered_map>
#include <stdlib.h>
#include <string.h>


using namespace std;
using namespace omnetpp;

int main(int argc, char **argv){

    std::string HOME(std::getenv("HOME"));
    std::string NEDPATH = "NEDPATH="+HOME + "/raynet/simulations:"+HOME+"/raynet/simlibs/RLComponents/src:"+HOME+"/raynet/simlibs/ecmp/src:"+HOME+"/raynet/simlibs/TcpPaced/src:"+HOME+"/raynet/simlibs/RLCC/src:"+HOME+"/raynet/simlibs/rdp/src:"+HOME+"/inet4.4/src/inet:"+HOME+"/inet4.4/examples";

    

    putenv(NEDPATH.c_str());
    // TODO: Initialise CmdRllibenv. This class will be bound to Python.

    std::string _iniPath;
    ObsType  obs;

    _iniPath = (string(getenv("HOME"))+string("/raynet/configs/ndpconfig_single_flow_eval_with_delay_template_debug.ini")).c_str();

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
    while (!done && strcmp(agentId.c_str(), "nostep") != 0) {
        for(std::unordered_map<std::string,ObsType>::iterator it = id_obs.begin(); it != id_obs.end(); ++it) {
            agentId = it->first;
            }

        std::cout << "Agent Id is:" << agentId << std::endl;
        std::cout << "Agent ID printed" << std::endl;
        std::unordered_map<std::string, ActionType> actions({ {agentId, 0} });
        auto ret = gymapi->step(actions);
        done = std::get<2>(ret)["__all__"];
        obs = std::get<0>(ret)[agentId];
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
