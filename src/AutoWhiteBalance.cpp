#include "FCam/AutoWhiteBalance.h"
#include "FCam/Sensor.h"
#include "FCam/Platform.h"
#include "Debug.h"


namespace FCam {

void autoWhiteBalance(Shot *s, const Frame &f, 
                      int minWB,
                      int maxWB,
                      float smoothness) {
    if (!s) return;

    if (!f.histogram().valid()) return;

    // auto-white-balance based on the histogram
    int buckets = f.histogram().buckets();

    // target wb
    int wb = 0;

    // Whitebalance for RGB sensor (e.g. N900)
    if (f.histogram().colorspace() == RGB) {
        // Compute the mean brightness in each color channel

        int rawRGB[] = {0, 0, 0};

        for (int b = 0; b < buckets; b++) {
            // Assume the color channels are GRBG (should really switch
            // based on the sensor instead)
            rawRGB[0] += f.histogram()(b, 0)*b;
            rawRGB[1] += f.histogram()(b, 1)*b;
            rawRGB[2] += f.histogram()(b, 2)*b;
        }

        // Solve for the linear interpolation between the RAW to sRGB
        // color matrices that makes red = blue. That is, we make the gray
        // world assumption.

        float RGB3200[] = {0, 0, 0};
        float RGB7000[] = {0, 0, 0};
        float d3200[12];
        float d7000[12];
        f.platform().rawToRGBColorMatrix(3200, d3200);
        f.platform().rawToRGBColorMatrix(7000, d7000);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RGB3200[i] += d3200[i*4+j]*rawRGB[j];
                RGB7000[i] += d7000[i*4+j]*rawRGB[j];
            }
        }

        float alpha = (RGB3200[2] - RGB3200[0])/(RGB7000[0] - RGB3200[0] + RGB3200[2] - RGB7000[2]);

        // inverse wb is used as the interpolant, so there's lots of 1./
        // in this formula to make the interpolant equal alpha as desired.
        wb = int(1./(alpha * (1./7000-1./3200) + 1./3200));
    }
    // Whitebalance using gamma corrected YUV histogram
    else if (f.histogram().colorspace() == YpUV) {
    
        // Compute average gamma corrected u,v
        int u = 0;              // accumulated values
        int v = 0;
        int samplesu = 0;       // number of samples
        int samplesv = 0;

        for (unsigned int i = 0; i < f.histogram().buckets(); i++) 
        {
            u += i*f.histogram()(i,1);
            v +=i*f.histogram()(i,2);

            samplesu += f.histogram()(i,1);
            samplesv += f.histogram()(i,2);
        }

        float avgU = (float) u / (float) samplesu;
        float avgV = (float) v / (float) samplesv;

        wb = f.whiteBalance();

        // The tolerance and step parameters are 
        // heuristically determined. TODO: calibration.
        float fTolerance = 5.0f;
        int step = 500;

        // In this color space gray is 128,128. 
        // Shift to the blues or the reds to try
        // to achieve a better wb
        if (avgU - avgV < -fTolerance) {
            wb -= step;
        } else if (avgU - avgV > fTolerance) {
            wb += step;
        } else {
            return;
        }
    }

    if (wb < minWB) wb = minWB;
    if (wb > maxWB) wb = maxWB;

    s->whiteBalance = smoothness * s->whiteBalance + (1-smoothness) * wb;   
}


}
