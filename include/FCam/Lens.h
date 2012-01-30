#ifndef FCAM_LENS_H
#define FCAM_LENS_H

//! \file 
//! An abstract base class for lenses

#include "Device.h"
#include "Action.h"

namespace FCam {

    /** An abstract base class for lens devices, to establish a uniform
        interface to common lens functions. */
    class Lens : public Device { 
    public: 

        /** @name Focus
         * \anchor Diopters
         *
         * Functions to affect the focus of the lens. Units are
         * diopters or diopters per second. Diopters are inverse
         * meters, so zero corresponds to infinity, and a large number
         * corresponds to very close. This is a good unit to use for
         * focus for many reasons: It accurately expresses the
         * precision with which focus is set; lens movements are often
         * linear in diopters; defocus blur size in pixels is
         * proportional to how many diopters you're misfocused by; and
         * depth of field is fixed number of diopters independent of
         * the depth you're focused at.
         */
        //@{
        /** Set the focus of the lens in diopters. 
         * 
         * See \ref Diopters "Focus" for a discussion of diopters.
         *
         * The second argument is the focus speed in diopters per
         * second. Numbers less than zero (like the default), get
         * mapped to max speed. This function initiates the change in
         * focus and returns. It may take some time before the focus
         * actually reaches the target position (or as close as it can
         * get). Use \ref FCam::Lens::focusChanging to see if the lens is
         * still moving.
         */
        virtual void setFocus(float, float speed = -1) = 0; 

        /** Get the current focus of the lens in diopters. */
        virtual float getFocus() const = 0;

        /** Get the farthest focus of the lens in diopters. */
        virtual float farFocus() const = 0;

        /** Get the closest focus of the lens in diopters. */
        virtual float nearFocus() const = 0;

        /** Is the focus currently changing? */
        virtual bool focusChanging() const = 0;

        /** if I call \ref FCam::Lens::setFocus "setFocus", how long
         * will it take before the lens starts moving? */
        virtual int focusLatency() const = 0;

        /** How slowly can I move the lens (in diopters per second) */
        virtual float minFocusSpeed() const = 0;

        /** How quickly can I move the lens (in diopters per second) */
        virtual float maxFocusSpeed() const = 0;
        //@}

        /** @name Zoom
         *
         * Functions to zoom the lens (change its focal length). The
         * units are focal length in mm, and mm per second.
         */
        //@{
        /** Initiate a move to the desired focal length. 
         * 
         * The second argument is the speed to move, in mm per second
         */
        virtual void setZoom(float, float speed = -1) = 0;

        /** The current focal length */
        virtual float getZoom() const = 0;

        /** The minimum focal length (widest-angle view) */
        virtual float minZoom() const = 0;

        /** The maximum focal length (narrowest-angle view) */
        virtual float maxZoom() const = 0;

        /** Is the focal length currently changing? */
        virtual bool zoomChanging() const = 0;

        /** How long after I call \ref FCam::Lens::setZoom "setZoom"
         * will the lens start moving? */
        virtual int zoomLatency() const = 0;

        /** The slowest the lens can zoom in mm per second */
        virtual float minZoomSpeed() const = 0;

        /** The fastest the lens can zoom in mm per second */
        virtual float maxZoomSpeed() const = 0;

        /** @name Aperture
         *
         * Functions to change the size of the aperture. The units are
         * F/numbers or F/numbers per second. To get the actual
         * physical size of the aperture, use
         * getFocalLength()/getAperture.
         */
        //@{
        /** Initiate a change in the aperture.
         *
         * The second argument is the desired speed with which to open
         * or close the aperture in F/numbers per second. I know of no
         * lenses that support this, so the second argument may
         * disappear from the API in the near future.
         */
        virtual void setAperture(float, float speed = -1) = 0;

        /** Get the current aperture */
        virtual float getAperture() const = 0;

        /** Get the widest aperture (smallest F/number) the lens
         * supports at a given focal length.
         */
        virtual float wideAperture(float zoom) const = 0;

        /** Get the narrowest aperture (largest F/number) the lens
         * supports at a given focal length.
         */
        virtual float narrowAperture(float zoom) const = 0;

        /** Is the aperture currently changing? */
        virtual bool apertureChanging() const = 0;      

        /** How long after I call setAperture does the aperture actually start moving? */
        virtual int apertureLatency() const = 0;

        /** The minimum speed with which the aperture can move in
         * F/numbers per second 
         */
        virtual float minApertureSpeed() const = 0;

        /** The maximum speed with which the aperture can move in
         * F/numbers per second
         */
        virtual float maxApertureSpeed() const = 0;
        //@}
    
        /** An Action to initiate a change in focus during an exposure
         * (for example, for rubber focus) */
        class FocusAction : public CopyableAction<FocusAction> {
        public:

        	virtual int type() const { return Action::LensFocus; }

            /** Make a new FocusAction associated with a particular lens. */
            FocusAction(Lens *);

            /** Make a new FocusAction to change the focus of the
             * given lens, at the given number of microseconds into
             * the exposure, towards the given focus. */
            FocusAction(Lens *, int, float);

            /** Make a new FocusAction to change the focus of the
             * given lens, at the given number of microseconds into
             * the exposure, towards the given focus, at the given
             * speed. */
            FocusAction(Lens *, int, float, float);

            /** The target focus in diopters */
            float focus;

            /** The speed at which to change focus in diopters per second */
            float speed;

            void doAction();
        private:
            Lens *lens;
        };

        /** An Action to perform a focus change in small steps in the 
         *  span of multiple frames. Usefull when streaming. After
         *  the number of steps have completed, the action becomes a no-op. */
        class FocusSteppingAction : public CopyableAction<FocusSteppingAction> {
        public:

        	virtual int type() const { return Action::LensStepFocus; }

            /** Make a new FocusSteppingAction associated with a particular lens. */
            FocusSteppingAction(Lens *);

            /** Make a new FocusSteppingAction to change the focus of the
             *  given lens with the given number of steps, executing each step 
             *  at the given number of microseconds into the exposure, 
             *  moving the lens by the given diopter step. */
            FocusSteppingAction(Lens *, int, int, float);

            /** Make a new FocusSteppingAction to change the focus of the
             *  given lens with the given number of steps, executing each step 
             *  at the given number of microseconds into the exposure, 
             *  moving the lens by the given diopter step, at the given speed */
            FocusSteppingAction(Lens *, int, int, float, float);

            /** The step to take in diopters */
            float step;

            /** The speed at which each step is taken, in diopters per second */
            float speed;

            /** The number of steps to take. After each step is taken,
              * repeat will be decremented by one. When it reaches 0, 
              * doAction becomes a no-op. */
            int   repeat;

            void doAction();
        private:
            Lens *lens;
        };


        /** An Action to initiate a change in zoom during an exposure
         * (for example, for zoom blur) */
        class ZoomAction : public CopyableAction<ZoomAction> {

        	virtual int type() const { return Action::LensZoom; }

            /** Make a new ZoomAction associated with a particular lens. */
            ZoomAction(Lens *);

            /** Make a new ZoomAction to change the focal length of
             * the given lens, at the given number of microseconds
             * into the exposure, towards the given focal length. */
            ZoomAction(Lens *, int, float);

            /** Make a new ZoomAction to change the focal length of
             * the given lens, at the given number of microseconds
             * into the exposure, towards the given focal length, at
             * the given speed. */
            ZoomAction(Lens *, int, float, float);

            /** The focal length to target in mm */
            float zoom;

            /** The speed at which to move there in mm per second */
            float speed;

            void doAction();
        private:
            Lens *lens;
        };

        /** An Action to initiate a change in aperture during an
         * exposure (for example, for apodization) */
        class ApertureAction : public CopyableAction<ApertureAction> {

        	virtual int type() const { return Action::LensAperture; }

            /** Make a new ApertureAction associated with a particular lens. */
            ApertureAction(Lens *);

            /** Make a new ApertureAction to change the aperture of
             * the given lens, at the given number of microseconds
             * into the exposure, to the given value. */
            ApertureAction(Lens *, int, float);

            /** Make a new ApertureAction to change the aperture of
             * the given lens, at the given number of microseconds
             * into the exposure, to the given value, at the given
             * speed. */
            ApertureAction(Lens *, int, float, float);
        
            /** The aperture to move to */
            float aperture;
        
            /** The speed with which to move the aperture */
            float speed;

            void doAction();
        private:
            Lens *lens;
        };

        /** Attach tags describing the state of the lens during a frame to a 
         * \ref Frame. You should never need to call this function - it's
         * done for you by the \ref Sensor. */
        virtual void tagFrame(Frame) = 0;

        /** A lens adds the following tags to a frame: "lens.focus",
         * "lens.focusSpeed", "lens.initialFocus", "lens.finalFocus",
         * "lens.zoom", "lens.zoomSpeed", "lens.initialZoom",
         * "lens.finalZoom", "lens.aperture", "lens.apertureSpeed",
         * "lens.initialAperture", "lens.finalAperture". They can be
         * retrieved by name from the frame, or you can construct a
         * \ref Lens::Tags object to grab them.
         *
         * A lens also adds several tags concerning its static
         * capabilities to each frame, not included in the Lens::Tags
         * object: "lens.minZoom", "lens.maxZoom",
         * "lens.wideApertureMin", and "lens.wideApertureMax". These
         * tags exist in order to place the correct lens metadata in
         * saved image files. minZoom and maxZoom refer to the zoom
         * range of the lens, and wideApertureMin and wideApertureMax
         * give the widest aperture settings at min and max zoom
         * respectively.
         */
        struct Tags {
            
            /** Construct a lens tags object from a frame. Tags can also be retrieved from a frame by name. */
            Tags(Frame);

            float focus;           //!< The average focus setting of the lens over the course of this frame.
            float focusSpeed;      //!< The average speed at which the focus was changing over the course of this frame.
            float initialFocus;    //!< The focus setting at the start of the frame.
            float finalFocus;      //!< The focus setting at the end of the frame.

            float zoom;            //!< The average zoom setting of the lens over the course of this frame.
            float zoomSpeed;       //!< The average speed at which the zoom was changing over the course of this frame.
            float initialZoom;     //!< The zoom setting at the start of the frame. 
            float finalZoom;       //!< The zoom setting at the end of the frame.

            float aperture;        //!< The average aperture setting of the lens over the course of this frame.
            float apertureSpeed;   //!< The average speed at which the aperture was changing over the course of this frame.
            float initialAperture; //!< The aperture setting at the start of the frame.
            float finalAperture;   //!< The aperture setting at the end of the frame.
        };
    };

}

#endif

