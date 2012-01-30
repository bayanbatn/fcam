#ifndef _HPT_H
#define _HPT_H

#include <sys/time.h>
#include <queue>

class Timer {
public:
	Timer(void) {
		gettimeofday(&m_startup, 0);
	}

	~Timer(void) {
	}

	void tic(void) {
		timeval tv;
		gettimeofday(&tv, 0);
		m_timeStampStack.push(tv);
	}

	double toc(void) {
		timeval end;
		gettimeofday(&end, 0);
		const timeval &start = m_timeStampStack.front();
		m_timeStampStack.pop();

		return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) * 0.001;
	}

	double get(void) {
		timeval tv;
		gettimeofday(&tv, 0);
		return (tv.tv_sec - m_startup.tv_sec) * 1000.0 + (tv.tv_usec - m_startup.tv_usec) * 0.001;
	}

private:
	timeval m_startup;
	std::queue<timeval> m_timeStampStack;
};

#endif

