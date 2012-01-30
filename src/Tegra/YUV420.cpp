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
#include "FCam/Tegra/YUV420.h"

namespace FCam { namespace Tegra {

static inline char clamp(int n, int min=1, int max=255)
{
    if (n < min)   return (char) min;
    if (n > max)   return (char) max;
    return (char) n;
}

bool convertYUV420ToRGB24(Image dst, Image im) {
    
    // Check src/dst compatibility
    if (im.size().width  != dst.size().width ||
        im.size().height != dst.size().height ||
        im.type() != YUV420p || dst.type() != RGB24) {
            return false;
    }

    Rect boundaries;
    boundaries.x = 0;
    boundaries.y = 0;
    boundaries.width = im.size().width;
    boundaries.height = im.size().height;

    for (int j = boundaries.y; j < boundaries.y + boundaries.height; j += 2)
    {
        unsigned int  uvrow     = j/4;
        unsigned int  uvcol     = boundaries.x/2 + boundaries.y%4 < 2 ? 0 : im.width()/2;
        unsigned char *rgb      = dst(boundaries.x, j);
        unsigned char *dataYPtr = im(boundaries.x, j);
        unsigned char *dataUPtr = im(uvcol, im.height() + uvrow);
        unsigned char *dataVPtr = im(uvcol, im.height() + im.height()/4 + uvrow);

        for (int i = boundaries.x; i < boundaries.x + boundaries.width; i += 2)
        {
            // formula
            // R = [((-1+149*y)/2 - (14216-102*v)     )/2]/32
            // G = [((-1+149*y)/2 + ((8696-25*u)-52*v))/2]/32
            // B = [((-1+149*y)/2 - (17672-129*u)     )/2]/32
            int y1 = dataYPtr[0];
            int y2 = dataYPtr[1];
            int y3 = dataYPtr[im.width()];
            int y4 = dataYPtr[im.width() + 1];
            int u  = dataUPtr[0];
            int v  = dataVPtr[0];

            y1 = (-1+149*y1)/2;
            y2 = (-1+149*y2)/2;
            y3 = (-1+149*y3)/2;
            y4 = (-1+149*y4)/2;
            int ruv = (14216-102*v);
            int guv = ((8696-25*u)-52*v);
            int buv = (17672-129*u);

            

            rgb[0] = clamp((y1 - ruv)/64);
            rgb[1] = clamp((y1 + guv)/64);
            rgb[2] = clamp((y1 - buv)/64);
            
            rgb[3] = clamp((y2 - ruv)/64);
            rgb[4] = clamp((y2 + guv)/64);
            rgb[5] = clamp((y2 - buv)/64);

            rgb[0+im.width()*3] = clamp((y3 - ruv)/64);
            rgb[1+im.width()*3] = clamp((y3 + guv)/64);        
            rgb[2+im.width()*3] = clamp((y3 - buv)/64);
            
            rgb[3+im.width()*3] = clamp((y4 - ruv)/64);
            rgb[4+im.width()*3] = clamp((y4 + guv)/64);
            rgb[5+im.width()*3] = clamp((y4 - buv)/64);
            
            rgb      += 6;
            dataYPtr += 2;
            dataUPtr += 1;
            dataVPtr += 1;
        }
    }

    return true;
}        


}}
