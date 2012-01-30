#include <stdlib.h>
#include "FCam/Time.h"
#include "Debug.h"


namespace FCam {

Time Time::now() {
    Time t2;
    gettimeofday(&t2.t, NULL);
    return t2;
}

int Time::operator-(const Time &other) const {
    return (((int)(s()) - (int)(other.s()))*1000000 + 
            ((int)(us()) - (int)(other.us())));
}

bool Time::operator<(const Time &other) const {
    return (s() < other.s() ||
            (s() == other.s() && us() < other.us()));
}

bool Time::operator>(const Time &other) const {
    return (s() > other.s() ||
            (s() == other.s() && us() > other.us()));
}

bool Time::operator>=(const Time &other) const {
    return (s() > other.s() ||
            (s() == other.s() && us() >= other.us()));
}

bool Time::operator<=(const Time &other) const {
    return (s() < other.s() ||
            (s() == other.s() && us() <= other.us()));
}

bool Time::operator==(const Time &other) const {
    return (s() == other.s() &&
            us() == other.us());
}

bool Time::operator!=(const Time &other) const {
    return (us() != other.us() ||
            s() != other.s());
}


Time Time::operator+=(int usecs) {
    int newUsecs = us() + usecs;
    int dSec = 0;
    while (newUsecs < 0) {
        dSec--;
        newUsecs += 1000000;
    }
    while (newUsecs > 1000000) {
        dSec++;
        newUsecs -= 1000000;            
    }
    t.tv_usec = newUsecs;
    t.tv_sec += dSec;    
    return *this;
}

Time Time::operator+(int usecs) const {
    Time t2 = *this;
    t2 += usecs;
    return t2;
}

Time::operator timeval() {
    return t;
}

Time::operator struct timespec() {
    struct timespec t_;
    t_.tv_sec = t.tv_sec;
    t_.tv_nsec = t.tv_usec*1000;
    return t_;
}

}
 
