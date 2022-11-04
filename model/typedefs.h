#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <tuple>

// Define the types for observation, action and reward.

typedef std::tuple<float, float, float, float, float, 
                   float, float, float, float, float, 
                   float, float, float, float, float,
                   float, float> ObsType;
typedef float RewardType;
typedef float ActionType;

#endif