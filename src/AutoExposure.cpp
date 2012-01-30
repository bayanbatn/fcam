#include <FCam/Frame.h>
#include <FCam/Sensor.h>
#include <FCam/Shot.h>

#include <FCam/AutoExposure.h>

#include "Debug.h"

namespace FCam {
    void autoExpose(Shot *s, const Frame &f,
                    float maxGain,
                    int maxExposure,
                    int minExposure,
                    float smoothness) {


        if (!s) return;

        int b = f.histogram().buckets();

        int cdf[256]; // a CDF for histograms of up to 256 buckets

        if (!f.histogram().valid()) return;
        if (b > 256 || b < 64) return;

        const Histogram &hist = f.histogram();

        cdf[0] = hist(0);
        for (int i = 1; i < b; i++) {
            cdf[i] = cdf[i-1] + hist(i);
        }

        int brightPixels = cdf[b-1] - cdf[b-21]; // top 20 buckets
        int targetBrightPixels = cdf[b-1]/50;
        int maxSaturatedPixels = cdf[b-1]/200;
        int saturatedPixels = cdf[b-1] - cdf[b-6]; // top 5 buckets

        // how much should I change brightness by
        float adjustment = 1.0f;

        if (saturatedPixels > maxSaturatedPixels) {
            // first don't let things saturate too much
            adjustment = 1.0f - ((float)(saturatedPixels - maxSaturatedPixels))/cdf[b-1];
        } else if (brightPixels < targetBrightPixels) {
            // increase brightness to try and hit the desired number of well exposed pixels
            int l = b-11;
            while (brightPixels < targetBrightPixels && l > 0) {
                brightPixels += cdf[l];
                brightPixels -= cdf[l-1];
                l--;
            }

            // that level is supposed to be at b-11;
            adjustment = float(b-11+1)/(l+1);
        } else {
            // we're not oversaturated, and we have enough bright pixels. Do nothing.
        }

        if (adjustment > 4.0) adjustment = 4.0;
        if (adjustment < 1/16.0f) adjustment = 1/16.0f;

        float brightness = f.gain() * f.exposure();
        float desiredBrightness = brightness * adjustment;
        int exposure;
        float gain;

        // Apply the smoothness constraint
        float shotBrightness = s->gain * s->exposure;
        desiredBrightness = shotBrightness * smoothness + desiredBrightness * (1-smoothness);

        // whats the largest we can raise exposure without negatively
        // impacting frame-rate or introducing handshake. We use 1/30s
        int exposureKnee = 33333;

        if (desiredBrightness > exposureKnee) {
            exposure = exposureKnee;
            gain = desiredBrightness / exposureKnee;
        } else {
            gain = 1.0f;
            exposure = desiredBrightness;
        }

        // Clamp the gain at max, and try to make up for it with exposure
        if (gain > maxGain) {
            exposure = desiredBrightness/maxGain;
            gain = maxGain;
        }

        // Finally, clamp the exposure at min/max.
        if (exposure > maxExposure) {
            exposure = maxExposure;
        }

        if (exposure < minExposure) {
            exposure = minExposure;
        }

        s->exposure  = exposure;
        s->gain      = gain;
    }
}
