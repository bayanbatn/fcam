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
#include "Statistics.h"

namespace FCam { namespace Tegra { 

FCam::SharpnessMap Statistics::evaluateSharpness(FCam::SharpnessMapConfig mapCfg, FCam::Image im)
{
    if (!mapCfg.enabled) return SharpnessMap();

    Rect region;
    region.x = 0;
    region.y = 0;
    region.width = im.size().width;
    region.height = im.size().height;

    // Make sure we don't take more than MAX_VARIANCE_SAMPLES samples
    // to avoid integer overflow.
    int subsample = 1;
    int samplesX, samplesY;

    samplesX = im.size().width;
    samplesY = im.size().height;
    while (samplesX * samplesY > MAX_VARIANCE_SAMPLES)
    {
        samplesX  = samplesX >> 1;
        samplesY  = samplesY >> 1;
        subsample = subsample << 1;
    }

    SharpnessMap s(Size(1,1), 1);
    s(0,0,0) = evaluateVariance(im, region, subsample);
    return s;
}

int Statistics::evaluateVariance(Image im, Rect region, int subsample)
{
    // Boundary check
    if (region.x < 0 || region.x >= im.size().width ||
        region.y < 0 || region.y >= im.size().height)
    {
        return 0.0f;
    }

    Rect boundaries = region;
    if (region.x + region.width >= im.size().width) 
    {
        boundaries.width = im.size().width - region.x - 1;
    }

    if (region.y + region.height >= im.size().height)
    {
        boundaries.height = im.size().height - region.y - 1;
    }
   
    int sumX2, sumX, samples;
    sumX2 = sumX = samples = 0;

    for (int j = boundaries.y; j < boundaries.y + boundaries.height; j += subsample)
    {
        unsigned char *dataPtr = im(boundaries.x, j);

        for (int i = boundaries.x; i < boundaries.x + boundaries.width; i += subsample) 
        {
            sumX    += dataPtr[0];
            sumX2   += dataPtr[0] * dataPtr[0];
            dataPtr += subsample;
            samples++;
        }
    }

    int expX2 = sumX2/samples;
    int expX  = sumX/samples;
    return    expX2 - expX * expX;
}


Histogram Statistics::evaluateHistogram(const HistogramConfig& histoCfg, const Image& im)
{

    unsigned buckets    = 1;
    unsigned reqbuckets = histoCfg.buckets;
    unsigned shiftbits = 0;
    unsigned bucketsize = 0;
    
    // Compute a power of two buckets from histogram config
    while(reqbuckets > 1) 
    {
        buckets = buckets << 1;
        reqbuckets = reqbuckets >> 1;
    }

    bucketsize = 256 / buckets;
    while (bucketsize > 1)
    {
        shiftbits++;
        bucketsize = bucketsize >> 1;
    }
    
    // Compute boundaries in uv plane (divide by 2)
    Rect boundaries = histoCfg.region;

    // Compute subsample factor
    int samplesX = boundaries.width  >> 1;
    int samplesY = boundaries.height >> 1;
    int subsample = 2; // uv are already subsampled by 2.
    while (samplesX * samplesY > MAX_HISTOGRAM_SAMPLES)
    {
        samplesX  = samplesX >> 1;
        samplesY  = samplesY >> 1;
        subsample = subsample << 1;
    }

    int subsample2 = subsample >> 1;
    Histogram histo(buckets, 3, histoCfg.region, YpUV);

    for (int j = boundaries.y; j < boundaries.y + boundaries.height; j += subsample)
    {
        unsigned int  uvrow     = j/4;
        unsigned int  uvcol     = boundaries.x/2 + boundaries.y%4 < 2 ? 0 : im.width()/2;
        unsigned char *dataYPtr = im(boundaries.x, j);
        unsigned char *dataUPtr = im(uvcol, im.height() + uvrow);
        unsigned char *dataVPtr = im(uvcol, im.height() + im.height()/4 + uvrow);

        for (int i = boundaries.x; i < boundaries.x + boundaries.width; i += subsample) 
        {           
            histo(dataYPtr[0] >> shiftbits, 0)++;
            histo(dataUPtr[0] >> shiftbits, 1)++;
            histo(dataVPtr[0] >> shiftbits, 2)++;

            dataYPtr += subsample;
            dataUPtr += subsample2;
            dataVPtr += subsample2;
        }
    }
    
    return histo;
}

}}
