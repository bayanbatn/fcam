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

#ifndef FCAM_TEGRA_AUTOFOCUS_H
#define FCAM_TEGRA_AUTOFOCUS_H

#include <vector>
#include "FCam/AutoFocus.h"

//! \file
//! Convenience functions for performing autofocus

namespace FCam { namespace Tegra {

    class Lens;

    /** This class does autofocus, by sweeping the sensor back and
        forth until a nice thing to focus on is found. You call \ref
        startSweep, then feed it frames using \ref update, until \ref
        focused returns true. */
    class AutoFocus : public FCam::AutoFocus {
    public:
        /** Construct an AutoFocus helper object that uses the
         * specified lens, and attempts to bring the target rectangle
         * into focus. By default, the target rectangle is the entire
         * region covered by the sharpness map of the frames that come
         * in. */
        AutoFocus(FCam::Tegra::Lens *l, FCam::Rect r = FCam::Rect());

        /** Start the autofocus routine. Returns immediately. */
        virtual void startSweep();

        /** Feed the autofocus routine a new frame. The sharpness map of the 
         * frame is examined, and the autofocus will react
         * appropriately. This function will ignore frames that aren't
         * tagged by the lens this object was constructed with, and
         * will also ignore frames with no sharpnes map.
         * The update routine will do a sweep in a single operation
         * by moving the lens at a low diopters/sec speed if possible. 
         * Otherwise, the range will be covered by mutiple short steps
         * at a high diopters/sec speed. In this latter case, the shot
         * s is updated with the corresponding FocusStepping action.
         */
        virtual void update(const FCam::Frame &f, FCam::Shot *s = NULL);

    protected:
        /** When state == SETTING || state == FOCUSED holds the
         *  best focus position.
         */
        Stats best;

    };

}}

#endif
