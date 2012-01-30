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
#include "../Debug.h"
#include <FCam/processing/Color.h>
#include <FCam/Tegra/Platform.h>

namespace FCam { namespace Tegra {

    Platform::Platform()
    {}
    
    Platform::~Platform()
    {}

    BayerPattern Platform::bayerPattern() const {
        return BGGR;
    }
    
    void Platform::rawToRGBColorMatrix(int kelvin, float *matrix) const {      

        // These are quick and dirty numbers computed using a random
        // outdoor overcast day, and interior office lighting.  The 3x3
        // linear portion of the matrices are scaled so that the minimum
        // row sum is one (i.e. saturated in all channels maps to white).
        static float RawToRGBColorMatrix3200K[] = {                
            1.6697,   -0.2693,   -0.4004,  -42.4346,
            -0.3576,    1.0615,    1.5949,  -37.1158,
            -0.2175,  -1.8751,    6.9640,  -26.6970
        };
        
        static float RawToRGBColorMatrix7000K[] = {
            2.2997,   -0.4478,    0.1706,  -39.0923,
            -0.3826,    1.5906,   -0.2080,  -25.4311,
            -0.0888,   -0.7344,    2.2832,  -20.0826
        };

        // Linear interpolation with inverse color temperature
        float alpha = (1./kelvin-1./3200)/(1./7000-1./3200);
        colorMatrixInterpolate(RawToRGBColorMatrix3200K,
                               RawToRGBColorMatrix7000K,
                               alpha, matrix);
    }

    const std::string &Platform::manufacturer() const {
        static std::string manuf("NVIDIA");
        return manuf;
    }

    const std::string &Platform::model() const {
        static std::string model("NVIDIA T30 ref board");
        return model;
    }
    
    unsigned short Platform::minRawValue() const {
        return 0;
    }

    unsigned short Platform::maxRawValue() const {
        return 959; //todo find out max
    }

    Platform &Platform::instance() {
        static Platform plat;
        return plat;
    }

}}
