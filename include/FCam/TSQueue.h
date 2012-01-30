#ifndef FCAM_TSQUEUE_H
#define FCAM_TSQUEUE_H

#include <deque>
#include <iterator>
#include <semaphore.h>
#include <pthread.h>

#include <errno.h>
#include <string.h>

#include "Base.h"

//#include "../../src/Debug.h"

/** \file 
 * A basic thread-safe queue using pthread mutexes to wrap a
 * C++ STL queue. Convenient to use for devices, etc, if they need a
 * background control thread. This could probably be done with no locks, 
 * since it's just mostly used as a producer-consumer queue. But this way
 * we don't have to worry about it, and lock contention has not appeared
 * to be a problem yet. 
 */

namespace FCam {

    /** Basic thread-safe consumer/producer queue. Safe for multiple
     * producers, but not for multiple consumers.  The consumer will
     * typically call wait(), pop(), front(), and pull, while
     * producers mostly uses push().
     */
    template<typename T>
    class TSQueue {
    public:
        TSQueue();
        ~TSQueue();

        /** A thread-safe iterator to access the queue. Constructing
         * this locks the queue.  Allows algorithms to be run on the queue atomically. */
        class locking_iterator;

        /** Add a copy of item to the back of the queue. */
        void push(const T& val);
        /** Remove the frontmost element from the queue. */
        void pop();

        /** Add a copy of item to the front of the queue. */        
        void pushFront(const T& val);
        /** Remove the backmost element from the queue. */
        void popBack();

        /** Return a reference to the frontmost element of the queue.
         * Behavior not defined if size() == 0 */
        T& front();
        const T& front() const;

        /** Return a reference to the backmost element of the queue.
         * Behavior not defined if size() == 0 */
        T& back();
        const T& back() const;

        /** Returns true if empty, false otherwise. */
        bool empty() const;
        /** Returns the number of items in the queue */
        size_t size() const;
        
        /** Waits until there are entries in the queue.
         * The optional timeout is in microseconds, zero means no timeout. */
        bool wait(unsigned int timeout=0);

        /** Waits for the queue not to be empty, and then copies the
         * frontmost item, then pops it from the queue and returns the
         * element copy. A convenience function, which is somewhat
         * inefficient if T is a large class (so use it on
         * pointer-containing queues, mostly). This is the only safe
         * way for multiple consumers to use the queue.
         */
        T pull();

        /** Pulls the backmost element off the queue. */
        T pullBack();

        /** Atomically either dequeue an item, or fail to do so. Does
         * not block. Returns whether it succeeded. */
        bool tryPull(T *);
        bool tryPullBack(T *);

        /** Create an iterator referring to the front of the queue and
         * locks the queue. Queue will remain locked until all the
         * locked_iterators referring to it are destroyed. */
        locking_iterator begin();
        /** Create an iterator referring to the end of the queue and
         * locks the queue. Queue will remain locked until all the
         * iterators referring to it are destroyed.  */
        locking_iterator end();
        /** Deletes an entry referred to by a locking_iterator. Like a
         * standard deque, if the entry erased is at the start or end
         * of the queue, other iterators remain valid. Otherwise, all
         * iterators and references are invalidated. Returns true if
         * entry is successfully deleted. Failure can occur if some
         * other thread(s) successfully reserved a queue entry/entries
         * before the locking iterator claimed the queue.*/
        bool erase(TSQueue<T>::locking_iterator);
        
    private:
        std::deque<T> q;
        mutable pthread_mutex_t mutex;
        sem_t *sem;

        friend class locking_iterator;
    };

    template<typename T>
    class TSQueue<T>::locking_iterator: public std::iterator< std::random_access_iterator_tag, T> {
    public:
        locking_iterator();
        locking_iterator(TSQueue<T> *, typename std::deque<T>::iterator i);
        locking_iterator(const TSQueue<T>::locking_iterator &);
        ~locking_iterator();
        locking_iterator& operator=(const TSQueue<T>::locking_iterator &);
      
        locking_iterator& operator++();
        locking_iterator operator++(int);
        locking_iterator& operator--();
        locking_iterator operator--(int);
      
        locking_iterator operator+(int);
        locking_iterator operator-(int);

        locking_iterator& operator+=(int);
        locking_iterator& operator-=(int);

        int operator-(const TSQueue<T>::locking_iterator &);

        bool operator==(const TSQueue<T>::locking_iterator &other);
        bool operator!=(const TSQueue<T>::locking_iterator &other);
        bool operator<(const TSQueue<T>::locking_iterator &other);
        bool operator>(const TSQueue<T>::locking_iterator &other);
        bool operator<=(const TSQueue<T>::locking_iterator &other);
        bool operator>=(const TSQueue<T>::locking_iterator &other);

        T& operator*();
        T* operator->();
    private:
        TSQueue *parent;
        typename std::deque<T>::iterator qi;
      
        friend class TSQueue<T>;
    };

    template<typename T>
    TSQueue<T>::TSQueue() {
        pthread_mutexattr_t mutexAttr;
        pthread_mutexattr_init(&mutexAttr);
        pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &mutexAttr);
#ifdef FCAM_PLATFORM_OSX
        // unnamed semaphores not supported on OSX
        char semName[256];
        // Create a unique semaphore name for this TSQueue using its pointer value
        snprintf(semName, 256, "FCam::TSQueue::sema::%llx", (long long unsigned)this);
        sem = sem_open(semName, O_CREAT, 0600, 0);
	if (sem == SEM_FAILED) {
            fcamPanic("TSQueue::TSQueue: Unable to initialize semaphore %s: %s", 
                      semName,
                      strerror(errno));
	}
#else
        sem = new sem_t;
        int success = sem_init(sem, 0, 0);
        if (success == -1) {
            fcamPanic("TSQueue::TSQueue: Unable to initialize semaphore: %s", 
                      strerror(errno));
        }
#endif
        //dprintf(6,"TSQueue %llx initialized\n", (long long unsigned)this);
    }
    
    template<typename T>
    TSQueue<T>::~TSQueue() {
        pthread_mutex_destroy(&mutex);       
#ifdef FCAM_PLATFORM_OSX
        int success = sem_close(sem);
        char semName[256];
        // Recreate the unique semaphore name for this TSQueue using its pointer value
        snprintf(semName, 256, "FCam::TSQueue::sema::%llx", (long long unsigned)this);
        if (success == 0) success = sem_unlink(semName);
        if (success == -1) {
            fcamPanic("TSQueue::~TSQueue: Unable to destroy semaphore %s: %s\n", 
                      semName, 
                      strerror(errno));
        }

#else
        int success = sem_destroy(sem);
        if (success == -1) {
            fcamPanic("TSQueue::~TSQueue: Unable to destroy semaphore: %s\n", 
                      strerror(errno));
        }
        delete sem;
#endif
    }
    
    template<typename T>
    void TSQueue<T>::push(const T& val) {
        //dprintf(6, "Pushing to queue %llx\n",(long long unsigned)this);
        pthread_mutex_lock(&mutex);
        q.push_back(val);
        pthread_mutex_unlock(&mutex);
        sem_post(sem);
    }

    template<typename T>
    void TSQueue<T>::pushFront(const T& val) {
        pthread_mutex_lock(&mutex);
        q.push_front(val);
        pthread_mutex_unlock(&mutex);
        sem_post(sem);
    }

    template<typename T>
    void TSQueue<T>::pop() {
        // Only safe for a single consumer!!!
        sem_wait(sem);
        pthread_mutex_lock(&mutex);
        q.pop_front();
        pthread_mutex_unlock(&mutex);
    }

    template<typename T>
    void TSQueue<T>::popBack() {
        // Only safe for a single consumer!!!
        sem_wait(sem);
        pthread_mutex_lock(&mutex);
        q.pop_back();
        pthread_mutex_unlock(&mutex);
    }   

    template<typename T>
    T& TSQueue<T>::front() {
        pthread_mutex_lock(&mutex);
        T &val = q.front();
        pthread_mutex_unlock(&mutex);
        return val;
    }

    template<typename T>
    const T& TSQueue<T>::front() const{
        const T &val;
        pthread_mutex_lock(&mutex);
        val = q.front();
        pthread_mutex_unlock(&mutex);
        return val;
    }

    template<typename T>
    T& TSQueue<T>::back() {
        pthread_mutex_lock(&mutex);
        T& val = q.back();
        pthread_mutex_unlock(&mutex);
        return val;
    }

    template<typename T>
    const T& TSQueue<T>::back() const {
        const T &val;
        pthread_mutex_lock(&mutex);
        val = q.back();
        pthread_mutex_unlock(&mutex);
        return val;
    }

    template<typename T>
    bool TSQueue<T>::empty() const {
        bool _empty;
        pthread_mutex_lock(&mutex);
        _empty = q.empty();
        pthread_mutex_unlock(&mutex);
        return _empty;
    }

    template<typename T>
    size_t TSQueue<T>::size() const {
        size_t _size;
        pthread_mutex_lock(&mutex);
        _size = q.size();
        pthread_mutex_unlock(&mutex);
        return _size;
    }

    template<typename T>
    bool TSQueue<T>::wait(unsigned int timeout) {
        bool res;
        int err;        
        if (timeout == 0) {
            err=sem_wait(sem);
        } else {
#ifndef FCAM_PLATFORM_OSX // No clock_gettime or sem_timedwait on OSX 
            timespec tv;
            // This will overflow around 1 hour or so of timeout
            clock_gettime(CLOCK_REALTIME, &tv);
            tv.tv_nsec += timeout*1000;
            tv.tv_sec += tv.tv_nsec / 1000000000;
            tv.tv_nsec = tv.tv_nsec % 1000000000;
            err=sem_timedwait(sem, &tv);
#else
            err=sem_trywait(sem);
#endif
        }
        if (err == -1) {
            switch (errno) {
            case EINTR:
            case ETIMEDOUT:
                res = false;
                break;
            default:
                //error(Event::InternalError, "TSQueue::wait: Unexpected error from wait on semaphore: %s", strerror(errno));
                res = false;
                break;
            }
        } else {
            res = true;
            sem_post(sem); // Put back the semaphore since we're not actually popping
        }

        return res;
    }

    template<typename T>
    T TSQueue<T>::pull() {
        //dprintf(6, "Pulling from queue %llx\n",(long long unsigned)this);
        sem_wait(sem);
        pthread_mutex_lock(&mutex);
        T copyVal = q.front();
        q.pop_front();
        pthread_mutex_unlock(&mutex);
        //dprintf(6, "Done pull from queue %llx\n",(long long unsigned)this);
        return copyVal;
    }

    template<typename T>
    T TSQueue<T>::pullBack() {
        sem_wait(sem);
        pthread_mutex_lock(&mutex);
        T copyVal = q.back();
        q.pop_back();
        pthread_mutex_unlock(&mutex);
        return copyVal;
    }
    
    template<typename T>
    bool TSQueue<T>::tryPull(T *ptr) {
        if (sem_trywait(sem)) return false;
        pthread_mutex_lock(&mutex);
        T copyVal = q.front();
        q.pop_front();
        pthread_mutex_unlock(&mutex);
        *ptr = copyVal;
        return true;
    }
    
    template<typename T>
    bool TSQueue<T>::tryPullBack(T *ptr) {
        if (sem_trywait(sem)) return false;
        pthread_mutex_lock(&mutex);
        T copyVal = q.back();
        q.pop_back();
        pthread_mutex_unlock(&mutex);
        *ptr = copyVal;
        return true;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator TSQueue<T>::begin() {
        return locking_iterator(this, q.begin());
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator TSQueue<T>::end() {
        return locking_iterator(this, q.end());
    }

    template<typename T>
    bool TSQueue<T>::erase(TSQueue<T>::locking_iterator li) {
        /* Since we're erasing, decrement semaphore. */
        if (sem_trywait(sem)) {
            return false;
        }
        q.erase(li.qi);
        return true;
    }

    template<typename T>
    TSQueue<T>::locking_iterator::locking_iterator() : parent(NULL), qi()
    {}

    template<typename T>
    TSQueue<T>::locking_iterator::locking_iterator(TSQueue<T> *p, 
                                                   typename std::deque<T>::iterator i) : 
        parent(p), qi(i) 
    {
        if (parent) {
            pthread_mutex_lock(&(parent->mutex));
        }
    }

    template<typename T>
    TSQueue<T>::locking_iterator::locking_iterator(const TSQueue<T>::locking_iterator &other):
        parent(other.parent), qi(other.qi)
    {
        if (parent) {
            pthread_mutex_lock(&(parent->mutex));
        }
    }

    template<typename T>
    TSQueue<T>::locking_iterator::~locking_iterator() {
        if (parent) {
            pthread_mutex_unlock(&(parent->mutex));
        }
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator &TSQueue<T>::locking_iterator::operator=(const TSQueue<T>::locking_iterator &other) {
        if (&other == this) return (*this);
        if (parent &&
            other.qi == qi) return (*this);
        
        if (parent) pthread_mutex_unlock(&(parent->mutex));
        parent = other.parent;
        qi = other.qi;
        if (parent) pthread_mutex_lock(&(parent->mutex));

    }

    template<typename T>
    typename TSQueue<T>::locking_iterator &TSQueue<T>::locking_iterator::operator++() {
        qi++;
        return (*this);
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator TSQueue<T>::locking_iterator::operator++(int) {
        typename TSQueue<T>::locking_iterator temp(*this);
        qi++;
        return temp;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator &TSQueue<T>::locking_iterator::operator--() {
        qi--;
        return (*this);
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator TSQueue<T>::locking_iterator::operator--(int) {
        typename TSQueue<T>::locking_iterator temp(*this);
        qi--;
        return temp;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator TSQueue<T>::locking_iterator::operator+(int n) {
        typename TSQueue<T>::locking_iterator temp(*this);
        temp+=n;
        return temp;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator operator+(int n, 
                                                    typename TSQueue<T>::locking_iterator l) {
        return l+n;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator TSQueue<T>::locking_iterator::operator-(int n) {
        typename TSQueue<T>::locking_iterator temp(*this);
        temp-=n;
        return temp;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator &TSQueue<T>::locking_iterator::operator+=(int n) {
        qi += n;
        return *this;
    }

    template<typename T>
    typename TSQueue<T>::locking_iterator &TSQueue<T>::locking_iterator::operator-=(int n) {
        qi -= n;
        return *this;
    }

    template<typename T>
    int TSQueue<T>::locking_iterator::operator-(const TSQueue<T>::locking_iterator &other) {
        return qi - other.qi;
    }
    
    template<typename T>
    bool TSQueue<T>::locking_iterator::operator==(const TSQueue<T>::locking_iterator &other) {
        return qi == other.qi;
    }

    template<typename T>
    bool TSQueue<T>::locking_iterator::operator!=(const TSQueue<T>::locking_iterator &other) {
        return qi != other.qi;
    }

    template<typename T>
    bool TSQueue<T>::locking_iterator::operator<(const TSQueue<T>::locking_iterator &other) {
        return qi < other.qi;
    }

    template<typename T>
    bool TSQueue<T>::locking_iterator::operator>(const TSQueue<T>::locking_iterator &other) {
        return qi > other.qi;
    }

    template<typename T>
    bool TSQueue<T>::locking_iterator::operator<=(const TSQueue<T>::locking_iterator &other) {
        return qi <= other.qi;
    }

    template<typename T>
    bool TSQueue<T>::locking_iterator::operator>=(const TSQueue<T>::locking_iterator &other) {
        return qi >= other.qi;
    }

    template<typename T>
    T& TSQueue<T>::locking_iterator::operator*() {
        return *qi;
    }

    template<typename T>
    T* TSQueue<T>::locking_iterator::operator->() {
        return &(*qi);
    }

}

#endif
