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
import inet.transportlayer.tcp.flavours.TcpCubicState;

namespace inet::tcp;


struct OrcaStateVariables extends TcpCubicStateVariables
{
   @descriptor(readonly);

uint64_t  avg_thr_raw = 0; 
uint64_t	avg_thr = 0;		
uint32_t	thr_cnt = 0;		
uint64_t  max_bw = 0;
simtime_t last_ack_arrival = 0;
uint64_t	pacing_rate = 0;
uint64_t	max_pacing_rate = 4294967295;
uint32_t  pre_lost_bytes = 0; 
uint32_t	lost_bytes = 0;			
uint32_t  orca_cnt = 0;		
uint32_t  min_rtt = 0;	
uint32_t  avg_urtt = 0;		
simtime_t last_mi_t = 0;

double delay_margin_coef = 1.25;

bool has_ss_finished = false;

};

cplusplus(OrcaStateVariables) {{
  public:
    virtual std::string str() const override;
    virtual std::string detailedInfo() const override;
}}