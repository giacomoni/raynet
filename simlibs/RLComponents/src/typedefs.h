#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <tuple>
#include <array>



// Define the types for observation, action and reward.

#ifdef RLRDP
typedef std::tuple<float, float, float, float, float, 
                   float, float, float, float, float, 
                   float, float, float, float, float,
                   float, float> ObsType;
typedef float RewardType;
typedef float ActionType;
#endif

#ifdef CARTPOLE
typedef std::array<double, 4> ObsType;
typedef int RewardType;
typedef int ActionType;
#endif



#endif