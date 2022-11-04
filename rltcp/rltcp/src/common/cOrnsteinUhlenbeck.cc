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

#include "cOrnsteinUhlenbeck.h"

namespace learning {

void cOrnsteinUhlenbeck::copy(const cOrnsteinUhlenbeck &other)
{
    theta = other.theta;
    mu = other.mu;
    sigma = other.sigma;
    dt = other.dt;
    x0 = other.x0;
    gamma = other.gamma;
    k = other.k;
}

cOrnsteinUhlenbeck& cOrnsteinUhlenbeck::operator=(const cOrnsteinUhlenbeck &other)
{
    if (this == &other)
        return *this;

    cRandom::operator=(other);
    copy(other);
    return *this;
}

std::string cOrnsteinUhlenbeck::str() const
{
    std::stringstream out;
    out << "theta=" << theta << ", mu=" << mu << ", sigma=" << sigma << ", dt=" << dt << ", x_0=" << x0 << ", gamma=" << gamma << ", k=" << k << std::endl;
    return out.str();
}

double cOrnsteinUhlenbeck::draw() const
{
    double x;
    x = xPrev + theta * (mu - xPrev) * dt + sigma * sqrt(dt) * normal(getRNG(), 0.0, (double) pow(gamma, k));
    xPrev = x;
    k++;
    return x;
}

}
