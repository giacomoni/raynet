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

import inet.common.INETDefs;
import inet.transportlayer.tcp_common.TcpHeader;
import inet.transportlayer.tcp.flavours.TcpTahoeRenoFamilyState;

namespace inet::tcp;


cplusplus {{
#include <common/utils/Estimator.h>
}}

class Estimator
{
   @existingClass;
   //FIXME tostring, ...
}




///
/// State variables for TcpTahoeRenoFamily.
///
struct RLTcpAlphaBetaStateVariables extends TcpTahoeRenoFamilyStateVariables
{
   @descriptor(readonly);
   
   double alpha = 1;
   double beta = 0.5;

   simtime_t last_ack_received = 0;
   uint64_t segments_lost = 0;
   uint64_t segments_lost_old = 0;

   Estimator bandwidthEstimator;
   Estimator rttEstimator;


};


