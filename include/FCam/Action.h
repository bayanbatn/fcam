#ifndef FCAM_ACTION_H
#define FCAM_ACTION_H

#include <stdio.h>

/** \file 
 * Abstract base classes for device actions.
 *
 * Actions represent things that devices should do at precise times
 * into the exposure. They are attached to \ref FCam::Shot objects.
 */ 

namespace FCam {
    /** An abstract base class for actions */
    class Action {
      public:

        Action() : owner(0) {}
        virtual ~Action();

        enum {
        	FlashMask = 0x1F,	// Bitmask to test for flash actions
        	FlashFire = 0x11,
        	FlashTorch,

        	LensMask  = 0x2F,	// Bitmask to test for lens actions
        	LensFocus = 0x21,
        	LensStepFocus,
        	LensZoom,
        	LensAperture,

        	CustomAction = 0xFFFF // Start of custom actions
        };

        /** A value that denotes the type of the action as defined by
         *  the enum.
         */
        virtual int type() const = 0;

        /** The number of microseconds into the exposure at which this
         * action should occur. */
        int time;
        
        /** How long before the \ref FCam::Action::time "time" must \ref FCam::Action::doAction "doAction" be called?    */
        int latency;

        /** An owner tag that indicates which module added the action to the shot.
         *  \ref FCam::Sensor::clearActions takes an owner tag to clear only actions
         *  that match the owner. Default is 0.
         */
        void *owner;

        /** Perform the action. Derived classes should override
         * this. This method will be called in the highest priority
         * thread possible to allow for precise timing, so don't
         * perform any long running computations in it unless you
         * really want the entire OS to freeze. */
        virtual void doAction() = 0;

        /** Make a new copy of this action (on the heap). Used when
         * making deep copies of Shot objects. Inherit from 
         * \ref CopyableAction instead to have this implemented for you
         * using your action's copy constructor.
         */
        virtual Action *copy() const = 0;        

      private:

        friend class Shot;
    };

    /** For convenience, derived \ref FCam::Action Action classes may
     * inherit from this so they don't have to implement the
     * \ref FCam::Action::copy method. It's templatized over the derived class
     * type.
     */
    template<typename Derived>
        class CopyableAction : public Action {
      public:
        /** Make a new copy of this action on the heap using the
         * default copy constructor. */
        Action *copy() const {
            return new Derived(*((Derived *)this));
        }
    };

}
    

#endif
