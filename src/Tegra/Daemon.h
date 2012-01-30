#ifndef FCAM_TEGRA_DAEMON_H
#define FCAM_TEGRA_DAEMON_H

#include <queue>
#include <pthread.h>
#include <semaphore.h>

#include "FCam/Frame.h"
#include "FCam/Tegra/Sensor.h"
#include "FCam/TSQueue.h"
#include "FCam/Tegra/Frame.h"

namespace FCam { namespace Tegra {

    // The daemon acts as a layer over the camera sw stack. It accepts frame
    // requests and returns frames that (hopefully) meet those requests.

    class Daemon {
    public:
        // A class for actions slaved to a frame
        class Action {
        public:
            // When should this action run?
            // The run-time will set this value when it knows when the
            // start of the associated exposure is
            Time time;
            FCam::Action *action;
                
            bool operator<(const Action &other) const {
                // I'm lower priority than another action if I occur later
                return time > other.time;
            }
                
            bool operator>(const Action &other) const {
                // I'm higher priority than another action if I occur earlier
                return time < other.time;
            }
        };
            
            
        Daemon(Sensor *sensor);
        ~Daemon();

        // enforce a drop policy on the frame queue
        void setDropPolicy(Sensor::DropPolicy p, int f);

        // The user-space puts partially constructed frames on this
        // queue. It is consumed by the setter thread.
        TSQueue<_Frame *> requestQueue;

        // The handler thread puts mostly constructed frames on this
        // queue. It is consumed by user-space.
        TSQueue<_Frame *> frameQueue;

        void launchThreads();

        void onFrame(Hal::CameraFrame* frame);
        void readyToCapture();

    private:

        // Access to the FCam sensor object
        Sensor *sensor;

        // Access to the OMX hardware interface
        Hal::ICamera *m_pCameraInterface;

        bool stop;

        // The frameQueue may not grow beyond this limit
        size_t frameLimit;

        // What should I do if the frame policy tries to grow beyond the limit
        Sensor::DropPolicy dropPolicy;
        void enforceDropPolicy();   

        // The setter thread puts in flight requests on this queue, which
        // is consumed by the handler thread
        TSQueue<_Frame *> inFlightQueue;
            
        // The setter thread also queues up RT actions on this priority
        // queue, which is consumed by the actions thread
        std::priority_queue<Action> actionQueue;
        pthread_mutex_t actionQueueMutex;
        sem_t actionQueueSemaphore;

        // The setter thread waits on this semaphore before issuing a request
        sem_t readySemaphore;

        // The component of the daemon that sets exposure and gain
        void runSetter();   
        _Frame *insertBubble(_Frame *params = NULL);

        pthread_t setterThread;
        void tickSetter(Time hs_vs);
        bool setterRunning;

        // The current state of the camera
        _Frame current;
        Shot lastGoodShot;

        // Some counters to keep track of exposure latency/gain latency.
        unsigned int exposureLatency;
        unsigned int gainLatency;

        // The component that executes RT actions
        void runAction();
        pthread_t actionThread;
        bool actionRunning;

        int daemon_fd;

        // Have the threads been launched?
        bool threadsLaunched;

        friend void *daemon_setter_thread_(void *arg);
        friend void *daemon_handler_thread_(void *arg);
        friend void *daemon_action_thread_(void *arg);
    };

}
}

#endif
