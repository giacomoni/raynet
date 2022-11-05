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

#include "rlUtil.h"

using namespace learning;


double learning::slope(const vector<double> &x, const vector<double> &y) {
    if (x.size() != y.size()) {
        throw cRuntimeError("Sizes of vectors x and y are different");
    }
    size_t n = x.size();

    ASSERT(n > 0);
    double avgX = 0.0;//accumulate(x.begin(), x.end(), 0.0) / n;
    double avgY = 0.0;//accumulate(y.begin(), y.end(), 0.0) / n;

    double numerator = 0.0;
    double denominator = 0.0;

    for (size_t i = 0; i < n; ++i) {
        numerator += (x[i] - avgX) * (y[i] - avgY);
        denominator += (x[i] - avgX) * (x[i] - avgX);
    }

    if (denominator == 0.0) {
        throw cRuntimeError("Denominator cannot be 0.");
    }

    if (isnan(numerator / denominator))
        EV_INFO << "Slope is nan";

    return numerator / denominator;
}

Observation::Observation(std::vector<double> _features)
{
    features = _features;
}

// checks whether each feature of the observation is valid
bool Observation::isValid()
{
    bool valid = true;
    for (double &feat : features) {
        if (isnan(feat) || feat > 1000000 || feat < -1000000) //validity check
            valid = false;
    }
    return valid;
}

double Observation::getFeature(int _index)
{
    return features.at(_index);
}

// returns an observation as string in the format [feat1, feat2, ..., featn]
string Observation::str() const
{
    string ret;
    ret += "[";
    size_t i = 0;
    for (double &f : features) {
        std::ostringstream streamObj;
        streamObj << std::fixed << std::setprecision(16) << f;
        ret += streamObj.str();
        i++;
        if (i < features.size())
            ret += ", ";
    }
    ret += "]";
    return ret;
}

State::State()
{
}

State::State(int _maxObservationsCount)
{
    this->maxObservationsCount = _maxObservationsCount;
}

void State::addObservation(Observation _obs)
{
    //if there is still free space in the vector, push back the new observation
    if (observations.size() < maxObservationsCount) {
        observations.push_back(_obs);
    }
    //otherwise delete the oldest element and add the new one.
    else {
        observations.erase(observations.begin());
        observations.push_back(_obs);
    }
}

vector<double> State::flatten()
{
    vector<double> ret;
    // check the size of an observation to infer the state space size
    // TODO: make this parameter an attribute
    int _stateSize = observations.at(0).features.size();

    // push back each feature of each observation in a single vector
    for (Observation &obs : observations) {
        for (double &f : obs.features) {
            ret.push_back(f);
        }
    }

    // make sure that if there are not enough observations to fill the state,
    // 0s are inserted instead (the NN model will have a fixed input size)
    while (ret.size() < maxObservationsCount * _stateSize) {
        ret.insert(ret.begin(), 0);
    }
    return ret;
}

bool State::isValid()
{
    bool valid = true;
    for (Observation &obs : observations) {
        if (!obs.isValid())
            valid = false;
    }
    return valid;
}

//return a string in the format [[feat1,feat2,..featn],[feat1,feat2,...,featn],...,[feat1,feat2,...,featn]]
string State::str() const
{
    string ret;
    ret += "[";
    size_t i = 0;
    if (observations.size() < maxObservationsCount) {
        for (size_t j = 0; j < maxObservationsCount - observations.size(); j++) {
            ret += "0, ";
        }
    }
    for (Observation &obs : observations) {
        ret += obs.str();
        i++;
        if (i < observations.size())
            ret += ", ";
    }
    ret += "]";
    return ret;
}

Step::Step()
{
}

Step::Step(State _s, double _a, double _r, State _s_p, bool _done)
{
    s = _s;
    a = _a;
    r = _r;
    s_p = _s_p;
    done = _done;
}

bool Step::isValid()
{
    bool valid = true;
    if (!s.isValid())
        valid = false;
    if (!s_p.isValid())
        valid = false;
    if (isnan(r) || r > 1000000 || r < -1000000)
        valid = false;
    if (isnan(a) || a > 1000000 || a < -1000000)
        valid = false;

    return valid;
}

string Step::str() const
{
    string ret;
    std::ostringstream streamObj, streamObj2;

    streamObj << std::fixed << std::setprecision(16) << a;
    streamObj2 << std::fixed << std::setprecision(16) << r;

    ret += "state: " + s.str() + "\naction: " + streamObj.str() + "\nreward: " + streamObj2.str() + "\nstate': " + s_p.str();

    return ret;
}

Query::Query(std::string _queryId, State _state) :
        queryId(_queryId), state(_state)
{

}

Response::Response(std::string _queryId, double _action) :
        queryId(_queryId), action(_action)
{

}

double AverageThroughput::compute(uint32_t _currentSndUna, simtime_t _now)
{
    //Throughput defined as the number of acknowledged bytes since last measurement divided by
    //time elapsed from last measurement

    double thr = (_currentSndUna - lastSndUna) / (_now - lastMeasured);
    lastMeasured = _now;
    lastSndUna = _currentSndUna;
    return thr;
}

const simtime_t& AverageThroughput::getLastMeasured() const
{
    return lastMeasured;
}

void AverageThroughput::setLastMeasured(const simtime_t &_lastMeasured)
{
    lastMeasured = _lastMeasured;
}

uint32_t AverageThroughput::getLastSndUna() const
{
    return lastSndUna;
}

void AverageThroughput::setLastSndUna(uint32_t _lastSndUna)
{
    lastSndUna = _lastSndUna;
}

void AverageRtt::addMeasurement(double _rtt)
{
    rtts.push_back(_rtt);
}

const std::vector<double>& AverageRtt::getDelays() const
{
    return rtts;
}

void AverageRtt::setDelays(const std::vector<double> &_rtts)
{
    this->rtts = _rtts;
}

double AverageRtt::compute()
{
    //Average all rtt measurements received.
    double total = 0;
    int count = 0;
    for (double &value : rtts) {
        total += value;
        count += 1;
    }

    rtts.clear();
    return total / count;
}

Ewma::Ewma(double alpha)
{
    this->alpha = alpha;
}

Ewma::Ewma(double alpha, double initialOutput)
{
    this->alpha = alpha;
    this->output = initialOutput;
    this->hasInitial = true;
}

void Ewma::reset()
{
    this->hasInitial = false;
}

double Ewma::filter(double input)
{
    if (hasInitial) {
        output = alpha * (input - output) + output;
    }
    else {
        output = input;
        hasInitial = true;
    }
    return output;
}

double Ewma::getAlpha() const
{
    return alpha;
}

void Ewma::setAlpha(double alpha)
{
    this->alpha = alpha;
}

double Ewma::getOutput() const
{
    return output;
}

void Ewma::setOutput(double output)
{
    this->output = output;
}



