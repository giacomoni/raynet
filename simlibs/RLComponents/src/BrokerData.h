#ifndef BROKERDATA_H
#define BROKERDATA_H


#include <omnetpp.h>
#include <string.h>
#include <array>
#include <algorithm>
#include <tuple>
#include "typedefs.h"

using namespace omnetpp;
using namespace std;


class BrokerData : public cObject, noncopyable
{
  private:
    bool reset; // is the instruction a reset or step event
    ActionType action; // the action given by the agent
    bool done; // the episode has terminated. Time to reset the env again
	  ObsType obs; // observation of the env
	  RewardType reward; //reward achieved by the previous environment
    bool valid = true; //Flag whether the observation should be used (Used by Orca to skip steps)
  
  public:
    BrokerData();
    ~BrokerData();
    ActionType getAction();
    bool isReset();
    void setReset(bool _reset);
    bool isValid();
    void setValid(bool _valid);
    void setAction(ActionType act);
    bool getDone();
    void setDone(bool finished);
    RewardType getReward();
    void setReward(RewardType reward);
    ObsType getObs();
    void setObs(ObsType state);
     
};

#endif