#ifndef FCAM_COLOR_H
#define FCAM_COLOR_H

/*! \file
 * Assorted color manipulation utilities.
 *
 * This file declares several functions to manipulate color temperatures
 * and color conversion matrices.
 */

namespace FCam {

    /** 3x3 conversion matrix from linear sRGB to CIE XYZ */
    const float RGBtoXYZ[9] = {
        0.4124564, 0.3575761, 0.1804375,
        0.2126729, 0.7151522, 0.0721750,
        0.0193339, 0.1191920, 0.9503041
    };

    /** CIE 1931 x,y to correlated color temperature approximation
     * function.  Uses approximate formula of Hernandez-Andres, et
     * al. 1999: "Calculating correlated color temperatures across the
     * entire gamut of daylight and skylight chromaticities" Accurate
     * within 2% inside 3K-50K CCT. */
    int xyToCCT(float x, float y);

    /** Blackbody radiator color temperature to CIE 1931 x,y chromaticity approximation function.
     * Uses approximate formula of Kim et al. 2002:
     * "Design of Advanced Color - Temperature Control System for HDTV Applications" and associated patent.
     */
    void kelvinToXY(int T, float *x, float *y);

    /** Compute a 3x3 matrix inverse. */
    void invert3x3(float *in, float *out);
    void invert3x3(double *in, double *out);

    /** Linearly interpolates two 3x4 color matrices a and b using
     * interpolant alpha.  (result = a if alpha =0, result = b if
     * alpha = 1).  Rescales result matrix to have a minimum row sum
     * of 1.
     */
    void colorMatrixInterpolate(const float *a, const float *b, float alpha, float *result);

}
#endif
