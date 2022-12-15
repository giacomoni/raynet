#include "Estimator.h"

Estimator::Estimator(){

    flushCounter = 0;
    this->setWindowSize(10);
}



void Estimator::setWindowSize(double _windowSize){
    windowSize = _windowSize;
}
void Estimator::addSample(double measurement, simtime_t timestamp){
    samples.emplace_back(measurement, timestamp);
    if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end() && (it->getTimestamp() + windowSize) < simTime())
        {
            // `erase()` invalidates the iterator, use returned iterator
            it = samples.erase(it);
     
         }
    }  
}

int Estimator::getFlushCounter(){
    return flushCounter;
}

void Estimator::flush(){
    flushCounter++;
    samples.clear();
}
unsigned int Estimator::getSize(){
    return samples.size();
}

double Estimator::getMean(){
    if (samples.empty()) {
        return 0;
    }
 
    double sum = 0.0;
    for (Measurement &i: samples) {
        sum += i.getMeasurement();
    }
    return sum / samples.size();
}


double Estimator::getMax(){
    if (samples.empty()) {
        return 0;
    }
 
    double _max = 0.0;
    for (Measurement &i: samples) {
        _max = std::max(_max, i.getMeasurement());
    }
    return _max;

}
double Estimator::getMin(){
     if (samples.empty()) {
        return 0;
    }
 
    double _min = 0.0;
    for (Measurement &i: samples) {
        if(_min == 0.0)
            _min = i.getMeasurement();

        _min = std::min(_min, i.getMeasurement());
    }
    return _min;
}

double Estimator::getStd(){

    if (samples.empty()) {
        return 0;
    }

    double mean = getMean();
    double variance = 0;

     for (Measurement &i: samples) {
        variance += pow(i.getMeasurement() - mean, 2);
    }

    return sqrt(variance / samples.size());
}

 double Estimator::getMean(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

     if (subsamples.empty()) {
        return 0;
    }
 
    double sum = 0.0;
    for (Measurement &i: subsamples) {
        sum += i.getMeasurement();
    }
    return sum / subsamples.size();


 }
 double Estimator::getMax(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

    if (subsamples.empty()) {
        return 0;
    }
 
    double _max = 0.0;
    for (Measurement &i: subsamples) {
        _max = std::max(_max, i.getMeasurement());
    }
    return _max;


 }
 double Estimator::getMin(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

     if (subsamples.empty()) {
        return 0;
    }
 
    double _min = 0.0;
    for (Measurement &i: subsamples) {
        if(_min == 0.0)
            _min = i.getMeasurement();

        _min = std::min(_min, i.getMeasurement());
    }
    return _min;


 }

 double Estimator::getStd(simtime_t subwindow){
     std::vector<Measurement> subsamples;
     if(!samples.empty()){
        auto it = samples.begin();
        while (it != samples.end())
        {
            if((it->getTimestamp() + subwindow) >= simTime())
                subsamples.push_back(*it);
            it++;
        }
    }  

    if (subsamples.empty()) {
        return 0;
    }

    double mean = getMean(subwindow);
    double variance = 0;

     for (Measurement &i: subsamples) {
        variance += pow(i.getMeasurement() - mean, 2);
    }

    return sqrt(variance / subsamples.size());


 }

std::vector<Measurement> Estimator::getSamples(){
    return samples;
}
