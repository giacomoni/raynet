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

package mltcp.transportlayer.rl;

import inet.transportlayer.tcp.Tcp;

simple RLTcp extends Tcp
{
    parameters:
        @class(RLTcp);

        int maxWindow; 					// maximum window size 
        int stateSize; 					// dimension of the state space
        int maxObsCount; 				// max number of observations for a single state
        double ewmaWeight; 				// weight for the moving average of state variables (as in Tcp-Ex Machina)
        double delayWeightReward; 		// weight for throughput vs. delay trade-off

        @signal[stepData];				// step data signal
        @signal[actionQuery];			// action query signal
}
