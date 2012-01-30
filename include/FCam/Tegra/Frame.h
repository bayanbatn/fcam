/* Copyright (c) 1995-2010, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* Copyright (c) 2011, NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived 
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef FCAM_TEGRA_FRAME_H
#define FCAM_TEGRA_FRAME_H

/** \file
 * The Tegra Frame class.
 */

#include "../Frame.h"
#include "Shot.h"
#include "Platform.h"

namespace FCam { namespace Tegra {

    struct _Frame : public FCam::_Frame {

        Shot _shot;

        /**
         * Set to true for frames captured in fastMode.
         * See FCam::Tegra::Sensor::fastMode()
         */
        bool fastMode;

        /** A bitwise value of Frame::FrameConfidence enums */
        unsigned int confidence;

        const Shot &shot() const { return _shot; }
        const FCam::Shot &baseShot() const { return shot(); }
        

        const FCam::Platform &platform() const {return ::FCam::Tegra::Platform::instance();}
    };

    /** The Tegra Frame class.
     *  Adds confidence() function
     */
    class Frame : public FCam::Frame {
    public:
        Frame(_Frame *f=NULL) : FCam::Frame(f) {}
        Frame(shared_ptr<FCam::_Frame> f) : FCam::Frame(f) {}
        ~Frame();

        /** Confidence enum defines a set values
         *  that can be AND together to indicate
         *  which returned parameters are reliable.
         */
        typedef enum
        {
            MATCH_REQUEST               = 0,
            UNCERTAIN_EXPOSURETIME      = 1,
            UNCERTAIN_GAIN              = 2,
        } Confidence;

        /** Set to true for frames captured in fastMode.
         *  Some frame parameters might not correspond to the
         *  requested ones in fastMode. Call confidence() to check which
         *  See FCam::Tegra::Sensor::fastMode()
         */
        bool fastMode() const { return static_cast<FCam::Tegra::_Frame*>(ptr.get())->fastMode; };

        /** Bitwise flags that indicate the confidence we have on the
         * parameters of this frame as defined by the Frame::Confidence enum.
         */
        unsigned int confidence() const { return static_cast<FCam::Tegra::_Frame*>(ptr.get())->confidence; };
    };
}}

#endif
