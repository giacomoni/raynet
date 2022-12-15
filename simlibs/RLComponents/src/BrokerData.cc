#include "BrokerData.h"

BrokerData::BrokerData()
{
}

BrokerData::~BrokerData()
{
}

ActionType BrokerData::getAction()
{
    return action;
}

bool BrokerData::isReset()
{
    return reset;
}

void BrokerData::setReset(bool _reset)
{
    reset = _reset;
}

bool BrokerData::isValid()
{
    return valid;
}

void BrokerData::setValid(bool _valid)
{
    valid = _valid;
}

void BrokerData::setAction(ActionType act)
{
    action = act;
}

bool BrokerData::getDone()
{
    return done;
}

void BrokerData::setDone(bool finished)
{
    done = finished;
}

RewardType BrokerData::getReward()
{
    return reward;
}

void BrokerData::setReward(RewardType award)
{
    reward = award;
}

ObsType BrokerData::getObs()
{
    return obs;
}

void BrokerData::setObs(ObsType state)
{
    obs = state;
}