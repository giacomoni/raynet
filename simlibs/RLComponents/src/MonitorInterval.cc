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

#include "MonitorInterval.h"

namespace learning {

MonitorInterval::MonitorInterval(uint32_t _miNumber, uint32_t _firstSeqSent, simtime_t _miStart, bool _isFirstPacketSent, bool _deferredTicking, double _action)
{
    ackForLastSegmentTime = SimTime::ZERO;
    miNumber = _miNumber;
    miStart = _miStart;
    firstSndMax = _firstSeqSent;
    isFirstSegmentSent =  _isFirstPacketSent;
    deferredTicking = _deferredTicking;
    action = _action;
    avgRtt = 0;

}

MonitorInterval::~MonitorInterval()
{
}

MonitorIntervalsHandler::MonitorIntervalsHandler()
{
    counter = 0;
}

MonitorIntervalsHandler::~MonitorIntervalsHandler()
{
}

void MonitorIntervalsHandler::addMI(shared_ptr<MonitorInterval> _mi)
{
    incompleteMis.push_back(_mi);
}
}
