//
// Copyright (C) 2020 Luca Giacomoni and George Parisis
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

#include <math.h>
#include <iomanip>
#include <vector>

#include <omnetpp.h>

//#include <ATen/core/Formatting.h>
//#include <torch/torch.h>

//#include <openssl/sha.h>

using namespace omnetpp;
using namespace std;

namespace learning {

double slope(const vector<double> &x, const vector<double> &y);

/*
 *  Represents a single Agent's observation of the environment on which it acts. An observation is a vector,
 *  whose elements represent the features of interest of the observed environment.
 *  Observations can be stacked (history) to compose a single state.
 */
struct Observation
{
    // the features/signals representing the state of the environment
    mutable vector<double> features;

    Observation(vector<double> _features);

    // checks that the value of every feature is valid (not Nan)
    bool isValid();
    double getFeature(int _index);

    // returns a string representation of the observation
    string str() const;
};

/*
 *  Multidimensional vector containing the agent's representation of the current environment's state. The state is composed of multiple (maxObservationsCount)
 *  observations of the environment. At every time step, the agent obtains an observation of the environment and pushes it back into the fixed size vector of
 *  observation. observations implements a FIFO logic.
 *
 */
struct State
{
    mutable std::vector<Observation> observations;  // FIFO vector of observations
    uint32_t maxObservationsCount;                  // maximum number of observation elements that observations can contain

    State();
    State(int _maxObservationsCount);

    // implements the FIFO logic when adding an element
    void addObservation(Observation _obs);

    // converts the multidimensional vector of size [maxObservationsCount, stateSize] into a single vector of size [maxObservationsCount * stateSize],
    // to be used as input of a neural network.
    vector<double> flatten();

    // checks that the value of every feature is valid (not Nan)
    bool isValid();

    // returns a string representation of the state
    string str() const;
};

/*
 * Represents a single interaction steps [S,A,R,s'], where S is the starting agent's representation of the environment's state, A is the action performed
 * given that state S, R is the reward returned by the environment and S' prime is the new agent's representation of the environment's state.
 * Step is used by DDPG (and other RL algorithm) as training data for the policy.
 */
struct Step : public cObject, noncopyable
{
    State s;    // inital state S
    double a;    // action A performed
    double r;    // reward R obtained
    State s_p;  // new state S'
    bool done;  // true if last step of episode

    Step();
    Step(State _s, double _a, double _r, State _s_p, bool _done);

    bool isValid();
    string str() const;
};

/*
 * cObject carried by the actionQuery Signal. It carries the state to be used as input for the agent's policy.
 */
struct Query : public cObject, noncopyable
{
    std::string queryId;                    // id of the query (echoed in the response)
    State state;                            // state for which the agent wants to know which action to take.

    Query(std::string _queryId, State _state);
};

/*
 * cObject carried by the actionResponse signal. It carries the action output by the policy for the input query with ID queryId
 */
struct Response : public cObject, noncopyable
{
    std::string queryId;                    // id echoed from the respective query
    double action;                           // action output by the agent's policy

    Response(std::string _queryId, double _action);
};

/*
 * Helper class to track the average throughput. Used by TcpRLReno to compute the average throughput at each time step.
 */
class AverageThroughput
{
private:
    simtime_t lastMeasured;    // last time the throughput was measured
    uint32_t lastSndUna;       // unacknowledged sequence number value stored when throughput was measured the last time.

public:
    // calculates the throughput achieved between last_measured and now.
    double compute(uint32_t _currentSndUna, simtime_t _now);

    const simtime_t& getLastMeasured() const;
    void setLastMeasured(const simtime_t &_lastMeasured);
    uint32_t getLastSndUna() const;
    void setLastSndUna(uint32_t _lastSndUna);
};

/*
 * Helper class to track the average rtt. Used by TcpRLReno to compute the average delay at each time step.
 */
class AverageRtt
{
    std::vector<double> rtts; //Vector containing perm packet rtt received in the last time-interval
public:
    // calculates the average rtt achieved in the last time interval.
    double compute();
    void addMeasurement(double rtt);
    const std::vector<double>& getDelays() const;
    void setDelays(const std::vector<double> &delays);
};

class Ewma
{
public:

    double output;        // current data value
    double alpha;         // smoothing factor, in range [0,1]. Higher the value - less smoothing (higher the latest reading impact).
    bool hasInitial = false; // check whether at least one value has been inserted

    // creates a filter without a defined initial output
    // the first output will be equal to the first input.
    Ewma(double alpha);

    // creates a filter with a defined initial output.
    Ewma(double alpha, double initialOutput);

    void reset();

    // return the new EWMA value given the input
    double filter(double input);

    double getAlpha() const;
    void setAlpha(double alpha = 0);

    double getOutput() const;
    void setOutput(double output = 0);

};

// Return the SHA as a string of content of replay buffer. Takes also in a vector containing the size pattern of the vector of tensor.
// The size patter is a list of the sizes of vector that repeat themeselves within the vector.
// In simplest case, the sizePattern will be: [3,1,1,3,1]
//std::string computeTensorVectorSHA(std::vector<torch::Tensor> &_tensorVec, std::vector<int> &sizePattern);

} //namespace learning
#endif /* COMMON_COMMON_H_ */
