#ifndef _WORKQUEUE_H
#define _WORKQUEUE_H

#include <pthread.h>
#include <semaphore.h>
#include <queue>

template<class T> class WorkQueue {
public:
	WorkQueue(void) {
		pthread_mutex_init(&m_lock, 0);
		sem_init(&m_counterSem, 0, 0);
	}

	~WorkQueue(void) {
		sem_destroy(&m_counterSem);
		pthread_mutex_destroy(&m_lock);
	}

	void produce(const T &elem) {
		pthread_mutex_lock(&m_lock);
		m_queue.push(elem);
		sem_post(&m_counterSem);
		pthread_mutex_unlock(&m_lock);
	}

	bool consume(T &elem, bool blocking = true) {
		if (blocking) {
			sem_wait(&m_counterSem);
		} else {
			if (sem_trywait(&m_counterSem) != 0) {
				return false;
			}
		}

		pthread_mutex_lock(&m_lock);
		elem = m_queue.front();
		m_queue.pop();
		pthread_mutex_unlock(&m_lock);

		return true;
	}

	void consumeAll(std::queue<T> &queue) {
		pthread_mutex_lock(&m_lock);
		queue = m_queue;
		if (!m_queue.empty()) {
			m_queue = std::queue<T>();
			// NVIDIA note: This assumes no concurrent call to consume. Fix?
			sem_destroy(&m_counterSem);
			sem_init(&m_counterSem, 0, 0);
		}
		pthread_mutex_unlock(&m_lock);
	}

	int size(void) {
		return m_queue.size();
	}

private:
	std::queue<T> m_queue;
	pthread_mutex_t m_lock;
	sem_t m_counterSem;
};

#endif

