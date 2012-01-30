#include <errno.h>
#include <iostream>

#include "FCam/AsyncFile.h"
#include "FCam/Frame.h"
#include "FCam/processing/JPEG.h"
#include "FCam/processing/DNG.h"
#include "FCam/processing/Dump.h"

#include "Debug.h"

using namespace std;

namespace FCam {
    void *launch_async_file_writer_thread_(void *arg) {
        AsyncFileWriter *d = (AsyncFileWriter *)arg;
        d->run();    
        d->running = false;    
        pthread_exit(NULL);
        return NULL;
    }


    AsyncFileWriter::AsyncFileWriter() {
        pthread_attr_t attr;
        struct sched_param param;

        pending = 0;
        
        // make the thread
        
        param.sched_priority = sched_get_priority_min(SCHED_OTHER);
        
        pthread_attr_init(&attr);

        running = false;
        stop = false;

#ifdef FCAM_PLATFORM_OSX
        // unnamed semaphores not supported on OSX
        char semName[256];
        // Create a unique semaphore name for this TSQueue using its pointer value
        snprintf(semName, 256, "FCam::AsyncFile::sem::%llx", (long long unsigned)this);
        saveQueueSemaphore = sem_open(semName, O_CREAT, 666, 0);
#else
        saveQueueSemaphore = new sem_t;
        sem_init(saveQueueSemaphore, 0, 0);
#endif
        pthread_mutex_init(&saveQueueMutex, NULL);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_OTHER) ||
#ifndef FCAM_PLATFORM_ANDROID
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
#endif
               pthread_create(&thread, &attr, launch_async_file_writer_thread_, this)))) {
            error(Event::InternalError, "Error creating async file writer thread");
            return;
        } else {
            running = true;
        }        
    }

    AsyncFileWriter::~AsyncFileWriter() {
        stop = true;
        sem_post(saveQueueSemaphore);
        if (running) {
            pthread_join(thread, NULL);
        }
#ifdef FCAM_PLATFORM_OSX
        sem_close(saveQueueSemaphore);
        char semName[256];
        // Recreate the unique semaphore name for this AsyncFileWriter using its pointer value
        snprintf(semName, 256, "FCam::AsyncFile::sem::%llx", (long long unsigned)this);
        sem_unlink(semName);
#else
        sem_destroy(saveQueueSemaphore);
        delete saveQueueSemaphore;
#endif

    }

    void AsyncFileWriter::saveDNG(Frame f, std::string filename) {
        pending++;
        SaveRequest r;
        r.frame = f;
        r.filename = filename;
        r.fileType = SaveRequest::DNGFrame;
        r.quality = 0; // meaningless for DNG

        pthread_mutex_lock(&saveQueueMutex);
        saveQueue.push(r);
        pthread_mutex_unlock(&saveQueueMutex);
        sem_post(saveQueueSemaphore);
    }

    void AsyncFileWriter::saveJPEG(Frame f, std::string filename, int quality) {
        pending++;
        SaveRequest r;
        r.frame = f;
        r.filename = filename;
        r.quality = quality;
        r.fileType = SaveRequest::JPEGFrame;

        pthread_mutex_lock(&saveQueueMutex);
        saveQueue.push(r);
        pthread_mutex_unlock(&saveQueueMutex);
        sem_post(saveQueueSemaphore);
    }

    void AsyncFileWriter::saveJPEG(Image im, std::string filename, int quality) {
        pending++;
        SaveRequest r;
        r.image = im;
        r.filename = filename;
        r.quality = quality;
        r.fileType = SaveRequest::JPEGImage;

        pthread_mutex_lock(&saveQueueMutex);
        saveQueue.push(r);
        pthread_mutex_unlock(&saveQueueMutex);
        sem_post(saveQueueSemaphore);
    }

    void AsyncFileWriter::saveDump(Frame f, std::string filename) {
        pending++;
        SaveRequest r;
        r.frame = f;
        r.filename = filename;
        r.quality = 0;
        r.fileType = SaveRequest::DumpFrame;

        pthread_mutex_lock(&saveQueueMutex);
        saveQueue.push(r);
        pthread_mutex_unlock(&saveQueueMutex);
        sem_post(saveQueueSemaphore);
    }

    void AsyncFileWriter::saveDump(Image im, std::string filename) {
        pending++;
        SaveRequest r;
        r.image = im;
        r.filename = filename;
        r.quality = 0;
        r.fileType = SaveRequest::DumpImage;

        pthread_mutex_lock(&saveQueueMutex);
        saveQueue.push(r);
        pthread_mutex_unlock(&saveQueueMutex);
        sem_post(saveQueueSemaphore);
    }

    void AsyncFileWriter::cancel() {
        pthread_mutex_lock(&saveQueueMutex);
        while (saveQueue.size()) {
            saveQueue.pop();
        };
        pthread_mutex_unlock(&saveQueueMutex);
    }

    void AsyncFileWriter::run() {
        while (!stop) {
            sem_wait(saveQueueSemaphore);
            if (stop) return;
            SaveRequest r;
            pthread_mutex_lock(&saveQueueMutex);
            r = saveQueue.front();
            saveQueue.pop();
            pthread_mutex_unlock(&saveQueueMutex);            
            switch (r.fileType) {
            case SaveRequest::DNGFrame:                    
                FCam::saveDNG(r.frame, r.filename);
                break;
            case SaveRequest::JPEGFrame:
                FCam::saveJPEG(r.frame, r.filename, r.quality);
                break;
            case SaveRequest::JPEGImage:
                FCam::saveJPEG(r.image, r.filename, r.quality);
                break;
            case SaveRequest::DumpFrame:
                FCam::saveDump(r.frame, r.filename);
                break;
            case SaveRequest::DumpImage:
                FCam::saveDump(r.image, r.filename);
                break;
            default:
                cerr << "Corrupted entry in async file writer save queue." << endl;
            }

            pending--;
        }
    }
}
    
