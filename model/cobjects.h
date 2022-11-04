#ifndef COBJECTS_H
#define COBJECTS_H

#include <omnetpp.h>
#include <string>

using namespace omnetpp;
using namespace std;

class cString: public cObject{
    public:
        std::string str;
        
        cString(std::string _str){str = _str;}
        cString(const char * _str){str = _str;}

};

class cSimTime: public cObject{
    public:
        simtime_t simtime;
        
        cSimTime(simtime_t _simtime){simtime = _simtime;}

};

#endif