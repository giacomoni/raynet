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

#include "TcpThroughputSinkAppThread.h"

Define_Module(TcpThroughputSinkAppThread);

void TcpThroughputSinkAppThread::initialize(int stage) {
    TcpSinkAppThread::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        throughputReceiverSignal = registerSignal("ReceiverSideThroughput");
        throughputTimer = new cMessage("THROUGHPUTTIMER");

        thrMeasurementInterval = par("thrMeasurementInterval");

        lastBytesReceived = 0;
        oldLastBytesReceived = 0;
    }

}

void TcpThroughputSinkAppThread::handleMessage(cMessage *message) {
    if (message == throughputTimer && message->isSelfMessage()) {
        EV_TRACE << "Message received at: " << simTime() << std::endl;
        double thr = computeThroughput(false);

        emit(throughputReceiverSignal, thr);

        oldLastBytesReceived = lastBytesReceived;
        lastBytesReceived = bytesRcvd;

        oldlastThroughputTime = lastThroughputTime;
        lastThroughputTime = simTime();

        scheduleAt(simTime() + thrMeasurementInterval, throughputTimer);
    }
}

void TcpThroughputSinkAppThread::established() {
    TcpSinkAppThread::established();

    lastThroughputTime = simTime();
    oldlastThroughputTime = simTime();

    EV_TRACE << "lastThroughputTime value set at: " << simTime() << std::endl;

    scheduleAt(simTime() + thrMeasurementInterval, throughputTimer);

}

double TcpThroughputSinkAppThread::computeThroughput(bool peerClosed) {
    double thr;
    if (!peerClosed) {
        EV_TRACE << "Bytes received since last measurement: " << bytesRcvd - lastBytesReceived << "B. Time elapsed since last time measured: " << simTime() - lastThroughputTime << std::endl;
        thr = (bytesRcvd - lastBytesReceived) * 8 / (simTime() - lastThroughputTime) / 1000000;
        EV_TRACE << "Throughput computed from application: " << thr << std::endl;
    }
    else {
        EV_TRACE << "Bytes received since last measurement: " << bytesRcvd - oldLastBytesReceived << "B. Time elapsed since last time measured: " << simTime() - oldlastThroughputTime << std::endl;
        thr = (bytesRcvd - oldLastBytesReceived) * 8 / (simTime() - oldlastThroughputTime) / 1000000;
        EV_TRACE << "Throughput computed from application: " << thr << std::endl;
    }
    return thr;
}

void TcpThroughputSinkAppThread::peerClosed() {
    TcpSinkAppThread::peerClosed();
    EV_TRACE << "Peer closed" << std::endl;
    double thr = computeThroughput(true);
    lastBytesReceived = bytesRcvd;
    lastThroughputTime = simTime();

    emit(throughputReceiverSignal, thr);
    if (throughputTimer->isScheduled()) {
        cancelEvent(throughputTimer);
    }

}

TcpThroughputSinkAppThread::~TcpThroughputSinkAppThread() {
    cancelAndDelete(throughputTimer);
}

