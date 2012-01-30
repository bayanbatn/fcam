#ifndef _TRIPLEBUFFER_H
#define _TRIPLEBUFFER_H

#include <pthread.h>

template<class T> class TripleBuffer {
public:
	TripleBuffer(T *buffers[3]) {
		pthread_mutex_init(&m_lock, 0);
		m_frontBuffer = buffers[0];
		m_backBuffer = buffers[1];
		m_spareBuffer = buffers[2];
		m_updateFrontBuffer = false;
	}

	~TripleBuffer(void) {
		pthread_mutex_destroy(&m_lock);
	}

	T *getFrontBuffer(void) {
		T *rval;

		pthread_mutex_lock(&m_lock);
		rval = m_frontBuffer;
		pthread_mutex_unlock(&m_lock);

		return rval;
	}

	T *swapFrontBuffer(void) {
		T *rval;

		pthread_mutex_lock(&m_lock);
		if (m_updateFrontBuffer) {
			rval = m_spareBuffer;
			m_spareBuffer = m_frontBuffer;
			m_frontBuffer = rval;
			m_updateFrontBuffer = false;
		} else {
			rval = m_frontBuffer;
		}
		pthread_mutex_unlock(&m_lock);

		return rval;
	}

	T *getBackBuffer(void) {
		T *rval;

		pthread_mutex_lock(&m_lock);
		rval = m_backBuffer;
		pthread_mutex_unlock(&m_lock);

		return rval;
	}

	T *swapBackBuffer(void) {
		T *rval;

		pthread_mutex_lock(&m_lock);
		rval = m_spareBuffer;
		m_spareBuffer = m_backBuffer;
		m_backBuffer = rval;
		m_updateFrontBuffer = true;
		pthread_mutex_unlock(&m_lock);

		return rval;
	}

private:
	T *m_frontBuffer, *m_backBuffer, *m_spareBuffer;
	bool m_updateFrontBuffer;
	pthread_mutex_t m_lock;
};

#endif

