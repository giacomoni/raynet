#ifndef __ESTIMATOR_H
#define __ESTIMATOR_H

#include <omnetpp.h>

using namespace omnetpp;

// Helper class storing a single measurement value and its timestmp
class Measurement{
    private:
        double measurement;
        simtime_t timestamp;
    public: 
        Measurement(double _measurement, simtime_t _timestamp){measurement=_measurement; timestamp=_timestamp;};
        double getMeasurement(){return measurement;};
        simtime_t getTimestamp(){return timestamp;};
};

class Estimator{
    private:
        double windowSize; // Size of the window in time (s)
        std::vector<Measurement> samples;
        int flushCounter;

    public:
        Estimator();
        void setWindowSize(double _windowSize);
        void addSample(double measurement, simtime_t timestamp);
        double getMean();
        double getMax();
        double getMin();
        double getStd();

        double getMean(simtime_t subwindow);
        double getMax(simtime_t subwindow);
        double getMin(simtime_t subwindow);
        double getStd(simtime_t subwindow);
        unsigned int getSize();
        void flush();
        int getFlushCounter();
        std::vector<Measurement> getSamples();
};


#endif 