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

#ifndef CORNSTEINUHLENBECK_H_
#define CORNSTEINUHLENBECK_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace learning {

/*
 * Implementation of OrnsteinUhlenbeck Noise. Used as exploration strategy during training.
 * It extends the cRandom class to be compatible with the current API and to have access to the RNG of Omnet++.
 * The random perturbation of this noise follows normal distribution with mean 0 and variance gamma ^ k.
 * Gamme is a value close to 1 (i.e. 0.999) and k is the episode/step number, depending on the application.
 */
class cOrnsteinUhlenbeck : public cRandom
{

private:
    void copy(const cOrnsteinUhlenbeck &other);

protected:
    double theta;   //nonnegative decay rate
    double mu;      //mean value of noise
    double sigma;   //measures the strength of the stochastic perturbation
    double dt;      //Time step between different samples
    double x0;      //Initial deviation from mean value
    double gamma;   //Base for the exponentially decayins variance of the normal distribution generating the perturbation
    mutable int k;  //Exponential for the exponentially decayins variance of the normal distribution generating the perturbation.
    mutable double xPrev;

public:
    cOrnsteinUhlenbeck(double _theta, double _mu, double _sigma, double _dt, double _gamma,double _x0, cRNG *rng) :
            cRandom(rng)
    {
        theta = _theta;
        mu = _mu;
        sigma = _sigma;
        dt = _dt;
        gamma = _gamma;
        x0 = _x0;
        k = 1;
        xPrev = x0;
    }

    cOrnsteinUhlenbeck(double _theta, double _mu, double _sigma, double _dt, double _gamma, double _x0, const char *name = nullptr, cRNG *rng = nullptr) :
            cRandom(name, rng)
    {
        theta = _theta;
        mu = _mu;
        sigma = _sigma;
        dt = _dt;
        gamma = _gamma;
        x0 = _x0;
        k = 1;
        xPrev = x0;
    }

    cOrnsteinUhlenbeck(const cOrnsteinUhlenbeck &other) :
            cRandom(other)
    {
        copy(other);
    }

    virtual cOrnsteinUhlenbeck* dup() const override
    {
        return new cOrnsteinUhlenbeck(*this);
    }

    ~cOrnsteinUhlenbeck()
    {
    }

    cOrnsteinUhlenbeck& operator =(const cOrnsteinUhlenbeck &other);

    virtual std::string str() const override;

    double getDt() const
    {
        return dt;
    }

    void setDt(double dt)
    {
        this->dt = dt;
    }

    double getGamma() const
    {
        return gamma;
    }

    void setGamma(double gamma)
    {
        this->gamma = gamma;
    }

    int getK() const
    {
        return k;
    }

    void setK(int k)
    {
        this->k = k;
    }

    double getMu() const
    {
        return mu;
    }

    void setMu(double mu)
    {
        this->mu = mu;
    }

    double getSigma() const
    {
        return sigma;
    }

    void setSigma(double sigma)
    {
        this->sigma = sigma;
    }

    double getTheta() const
    {
        return theta;
    }

    void setTheta(double theta)
    {
        this->theta = theta;
    }

    double get0() const
    {
        return x0;
    }

    void set0(double _0)
    {
        x0 = _0;
    }

    double getPrev() const
    {
        return xPrev;
    }

    void setPrev(double prev)
    {
        xPrev = prev;
    }

    //overrides the abstract method in cRandom. Return a random value following the OU random process.
    virtual double draw() const override;
};
}

#endif
