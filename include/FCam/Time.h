#ifndef FCAM_TIME_H
#define FCAM_TIME_H

//! \file 
//! The Time class encapsulates a wall clock time. 

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string>

namespace FCam {

    /** Time represents a wall clock time. Not to be used for
     * representing a duration of time. Two times can be subtracted to
     * return the difference between them in microseconds. A number of
     * microseconds can be added to or subtracted from time to return
     * a new time. Times also support all the comparison operators.
     */

    class Time {
    public:

        /** The current time */
        static Time now();
        
        /** Construct a Time from a number of seconds and microseconds */
        Time(int s, int us) {t.tv_sec = s; t.tv_usec = us;}

        /** Construct a Time from a timeval */
        Time(timeval t_) : t(t_) {};
           
        /** Construct a Time from a struct timespec */
        Time(struct timespec t_) {
            t.tv_sec = t_.tv_sec;
            t.tv_usec = t_.tv_nsec/1000;
        }

        Time()  {t.tv_sec = 0; t.tv_usec = 0;}
        
        /** The number of seconds since the epoch */
        int s() const {return t.tv_sec;}

        /** The number of microseconds since the last second */
        int us() const {return t.tv_usec;}

        std::string toString() const {
            time_t tim = t.tv_sec;
            struct tm *local = localtime(&tim);
            char buf[32];
            // From most significant to least significant
            snprintf(buf, 23, "%04d.%02d.%02d_%02d.%02d.%02d.%02d", 
                     local->tm_year + 1900,
                     local->tm_mon+1,
                     local->tm_mday,
                     local->tm_hour,
                     local->tm_min,
                     local->tm_sec,
                     (int)(t.tv_usec/10000));
            return std::string(buf);
        }
        
        /** @name Arithmetic
         *
         * You can add or subtract a number of microseconds to a time
         * to create a nearby time, or subtract to times to get the
         * difference in microseconds.
         */        
        //@{
        Time operator+(int usecs) const;
        Time operator+=(int usecs);
        Time operator-(int usecs) const {return (*this) + (-usecs);}
        Time operator-=(int usecs) {return (*this) += (-usecs);}
        int operator-(const Time &other) const;
        //@}

        /** @name Comparison
         *
         * Times can be compared using the standard operators
         */
        //@{
        bool operator<(const Time &other) const; 
        bool operator>(const Time &other) const;
        bool operator>=(const Time &other) const;
        bool operator<=(const Time &other) const;
        bool operator==(const Time &other) const;
        bool operator!=(const Time &other) const;
        //@}

        /** @name Casting
         *
         * Time can be cast to a timeval or struct timespec for use in
         * syscalls. 
         */
        //@{ 
        operator timeval();
        operator struct timespec(); 
        //@}

    private:
        timeval t;
    };
    
}

#endif
