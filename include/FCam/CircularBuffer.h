#ifndef FCAM_CIRCULAR_BUFFER_H
#define FCAM_CIRCULAR_BUFFER_H

#include <stdlib.h>

namespace FCam {

// A circular buffer to be used as a memory of recent events. You push
// stuff on, and then ask for the nth last thing you pushed on using
// operator[]. Giving operator[] an argument greater than the size
// returns a bad element.
template<typename T>
class CircularBuffer {
public:
    CircularBuffer(size_t s) {
        allocated = s;
        memory = new T[s];
        start = end = 0;
    }

    ~CircularBuffer() {
        delete[] memory;
        memory = NULL;
        start = end = allocated = 0;
    }

    T &operator[](size_t i) {
        return memory[(end - 1 - i + allocated) % allocated];
    }

    const T &operator[](size_t i) const {
        return memory[(end - 1 - i + allocated) % allocated];
    }

    size_t size() const {
        if (end >= start) return end-start;
        else return end - start + allocated;
    }

    void push(T obj) {
        memory[end] = obj;
        end++;
        if (end == allocated) end = 0;

        // if we ate our tail, update the start pointer too
        if (end == start) {
            start++;
            if (start == allocated) start = 0;
        }
    }


private:
    size_t start, end;
    size_t allocated;
    T *memory;
};

}

#endif
