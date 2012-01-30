#ifndef _PARAMSTAT_H
#define _PARAMSTAT_H

#include <math.h>

class ParamStat {
public:
	ParamStat(void) {
		reset();
	}

	~ParamStat(void) { }

	double getMean(void) {
		if (m_counter == 0) {
			return 0.0;
		}

		return m_accum / m_counter;
	}

	double getStdDev(void) {
		if (m_counter == 0) {
			return 0.0;
		}

		double mean = m_accum / m_counter;
		return sqrt(m_squareAccum / m_counter - mean * mean);
	}

	void reset(void) {
		m_accum = m_squareAccum = 0.0;
		m_counter = 0;
	}

	void update(double value) {
		m_accum += value;
		m_squareAccum += value * value;
		m_counter++;
	}

private:
	double m_accum, m_squareAccum;
	int m_counter;
};

#endif

