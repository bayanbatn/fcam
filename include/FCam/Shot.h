#ifndef FCAM_SHOT_H
#define FCAM_SHOT_H

//! \file 
//! Shot collects parameters for capturing a frame

#include "Base.h"
#include "Histogram.h"
#include "SharpnessMap.h"
#include "Image.h"
#include <set>
#include <pthread.h>

namespace FCam {

    class Action;

    /*! Shot collects parameters for capturing a frame.
     * It may also contain actions that are taken during a shot.
     * A returned Frame contains its requesting shot, whose ID will match with the shot passed in 
     * to a Sensor::capture() or Sensor::stream() call. Note that making a copy of a Shot, either by
     * assignment or a copy constructor assigns a new ID to the copy. This lets you say things like:
     *
     * Shot a;
     * .. set many fields of a ..
     * Shot b = a;
     * .. only adjust changed fields of b ..
     *
     * and still be able distinguish Frames created by a and b shots when they are handed over by the sensor.
     */
    class Shot {
      public:
        /** Target image. Set it to an allocated image to have it dump
         * the data into that image. To tell the system to discard the
         * image data, set the image's data pointer to Image::Discard
         * (using the optional last argument in the Image
         * constructor). To tell the system to allocate a new image of
         * the appropriate size for each new frame, set the image's data
         * pointer to Image::AutoAllocate
         */
        Image image;
        
        /** Requested exposure time in microseconds. */
        int exposure;  
        
        /** Requested amount of time the frame should take in
         * microseconds. You can set this to achieve a desired frame
         * rate. If this is less than the exposure time, then the
         * implementation will attempt to take the minimum amount of
         * time possible for the shot, so you can leave it at zero
         * (it's default value) if you just want the shot to happen as
         * quickly as possible. */
        int frameTime; 
            
        /** Gain for the shot. A value of 1.0 indicates no gain should
         * be applied, and will be the minimum for most
         * sensors. Values more than 1.0 will first use analog gain,
         * and then digital gain once analog gain is exhausted. */
        float gain; 

        /** Requested white-balance setting in Kelvin. Not applicable
            if the output format is raw. */
        int whiteBalance;
        
        /** The desired histogram generator configuration */
        HistogramConfig histogram;     

        /** The desired sharpness map generator configuration */
        SharpnessMapConfig sharpness;  
        
        /** Whether or not this shot should result in a frame. The
         * default is true, but if you don't want a frame to come back
         * at all for a given shot, you can set it to false. */
        bool wanted;

        /** Add an action to be performed during the shot. */
        void addAction(const Action &);

        /** Clear the set of actions to be performed during the shot for the given
         *  owner */
        void clearActions(void *owner = 0);

        /** Clears the set of actions to be performed during the shot. Note:
         *  some algorithms might have added actions to the shot and cleaning them
         *  will make them stop working. Consider using clearActions instead. */
        void clearAllActions();


        /** Acquire a const reference to the set of actions to be
         * performed during the short. */
        const std::set<Action *> &actions() const {return _actions;}

        /** A unique ID, generated on construction. Feel free to
         * replace it with your own identifier. */
        int id;

        Shot();
        ~Shot();

        /** Copying a shot results in a very deep copy. This assigns a
         * freshly generated shot id to the result. */
        Shot(const Shot &other);

        /** Copying a shot results in a very deep copy. This assigns a
         * freshly generated shot id to the result. */
        const Shot &operator=(const Shot &other);

        /** Set a custom color matrix to use to post-process this
         * shot. Expects a 3x4 matrix specified in row-major
         * order. This overrides any white balance set in the
         * whiteBalance field. */
        void setColorMatrix(const float *);
        void setColorMatrix(const std::vector<float> &vec) {setColorMatrix(&vec[0]);}

        /** If a custom color matrix has been set for this shot, this
         * returns it. Otherwise this will return an empty vector. */
        const std::vector<float> &colorMatrix() const {return _colorMatrix;}

        /** Clear any custom color matrix set for this shot. The
         * whiteBalance field will be used instead to determine the
         * color matrix. */
        void clearColorMatrix();

      private:
        static int _id;
        static pthread_mutex_t _idLock;        

        /** The set of actions slaved to this device. */
        std::set<Action *> _actions;

        /** A custom color matrix for this shot. */
        std::vector<float> _colorMatrix;
    };

}
#endif
