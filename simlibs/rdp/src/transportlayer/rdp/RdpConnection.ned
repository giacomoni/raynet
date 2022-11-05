//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package rdp.transportlayer.rdp;

simple RdpConnection
{
    parameters:
        @class("RdpConnection");
        @signal[state];    // FSM state
        @signal[trimmedHeaders];
        @signal[cwnd];
        @signal[ssthresh];

        //NdpAlgorithm signals:
        @signal[rtt];    // will record measured RTT
        @signal[srtt];    // will record smoothed RTT
        @signal[rttvar];    // will record RTT variance (rttvar)
		
        @statistic[state](record=vector; interpolationmode=sample-hold);
        @statistic[trimmedHeaders](record=vector; interpolationmode=sample-hold); 
		
		//CC statistics
		@statistic[cwnd](record=vector; interpolationmode=sample-hold);	   // number of trimmed headers
		@statistic[ssthresh](record=vector; interpolationmode=sample-hold);
		
        @statistic[rtt](record=vector; interpolationmode=sample-hold);    // will record measured RTT
        @statistic[srtt](record=vector; interpolationmode=sample-hold);    // will record smoothed RTT
        @statistic[rttvar](record=vector; interpolationmode=sample-hold);    // will record RTT variance (rttvar)
}
