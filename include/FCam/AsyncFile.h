#ifndef FCAM_ASYNCFILE_H
#define FCAM_ASYNCFILE_H

#include <queue>
#include <semaphore.h>
#include <string>
#include <pthread.h>

#include "Frame.h"

//! \file 
//! AsyncFile contains classes to load and save images in the background

namespace FCam {

    class Lens;
    class Flash;

    /** The AsyncFileWriter saves frames in a low priority background
     * thread */
    class AsyncFileWriter {
      public:
        AsyncFileWriter();
        ~AsyncFileWriter();

        /** Save a DNG in a background thread. */
        void saveDNG(Frame, std::string filename);
        void saveDNG(Image, std::string filename);

        /** Save a JPEG in a background thread. You can optionally
         * pass a jpeg quality (0-100). */
        void saveJPEG(Frame, std::string filename, int quality = 75);
        void saveJPEG(Image, std::string filename, int quality = 75);

        /** Save a raw dump in a background thread. */
        void saveDump(Frame, std::string filename);
        void saveDump(Image, std::string filename);

        /** How many save requests are pending (including the one
         * currently saving) */
        int savesPending() {return pending;}        

        /** Cancel all outstanding requests. The writer will finish
         * saving the current request, but not save any more */
        void cancel();

      private:

        friend void *launch_async_file_writer_thread_(void *);
        
        struct SaveRequest {
            // only one of these two things should be defined
            Frame frame;
            Image image;

            std::string filename;
            enum {DNGFrame = 0, JPEGFrame, JPEGImage, DumpFrame, DumpImage} fileType;
            int quality;
        };

        std::queue<SaveRequest> saveQueue;
        pthread_mutex_t saveQueueMutex;
        sem_t *saveQueueSemaphore;

        bool running, stop;
        pthread_t thread;

        void run();

        int pending;
    };

}

#endif
