#include <cmath>

#include <FCam/Event.h>

namespace FCam {

    int xyToCCT(float x, float y);
    int xyToCCT(float x, float y) {
        const float x_e = 0.3366;
        const float y_e = 0.1735;
        const float A0 = -949.86315;
        const float A1 = 6253.80338;
        const float t1 = 0.92159;
        const float A2 = 28.70599;
        const float t2 = 0.20039;
        const float A3 = 0.00004;
        const float t3 = 0.07125;

        float n = (x - x_e)/(y - y_e);

        float CCT = A0 + A1 * std::exp(-n/t1) + A2 * std::exp(-n/t2) + A3 * std::exp(-n/t3);
        if (CCT < 3000 || CCT > 50000) {
            warning(Event::OutOfRange, "xyToCCT: Conversion only accurate within 3000 to 50000K, result was %d K.\n", CCT);
        }
        return std::floor(CCT+0.5f);
    }

    void kelvinToXY(int T, float *x, float *y);
    void kelvinToXY(int T, float *x, float *y) {
        if (!x || !y) return;

        if (T < 1667 || T > 25000) {
            warning(Event::OutOfRange, "KelvinToXY: Conversion only accurate within 1667K to 25000K, requested %d K\n", T);
        }
        // chromaticity x coefficients for T <= 4000K
        const float A_x00 = -0.2661239;
        const float A_x01 = -0.2343580;
        const float A_x02 = 0.8776956;
        const float A_x03 = 0.179910;
        // chromaticity x coefficients for T > 4000K
        const float A_x10 = -3.0258469;
        const float A_x11 = 2.1070379;
        const float A_x12 = 0.2226347;
        const float A_x13 = 0.24039;
        // chromaticity y coefficients for T <= 2222K
        const float A_y00 = -1.1063814;
        const float A_y01 = -1.34811020;
        const float A_y02 = 2.18555832;
        const float A_y03 = -0.20219683;
        // chromaticity y coefficients for 2222K < T <= 4000K
        const float A_y10 = -0.9549476;
        const float A_y11 = -1.37418593;
        const float A_y12 = 2.09137015;
        const float A_y13 = -0.16748867;
        // chromaticity y coefficients for T > 4000K
        const float A_y20 = 3.0817580;
        const float A_y21 = -5.87338670;
        const float A_y22 = 3.75112997;
        const float A_y23 = -0.37001483;

        float invKiloK = 1000.f/T;
        float xc, yc;
        if (T <= 4000) {
            xc = A_x00*invKiloK*invKiloK*invKiloK +
                A_x01*invKiloK*invKiloK +
                A_x02*invKiloK +
                A_x03;
        } else {
            xc = A_x10*invKiloK*invKiloK*invKiloK +
                A_x11*invKiloK*invKiloK +
                A_x12*invKiloK +
                A_x13;
        }

        if (T <= 2222) {
            yc = A_y00*xc*xc*xc +
                A_y01*xc*xc +
                A_y02*xc +
                A_y03;
        } else if (T <= 4000) {
            yc = A_y10*xc*xc*xc +
                A_y11*xc*xc +
                A_y12*xc +
                A_y13;
        } else {
            yc = A_y20*xc*xc*xc +
                A_y21*xc*xc +
                A_y22*xc +
                A_y23;
        }

        *x = xc;
        *y = yc;

    }

    void invert3x3(float *in, float *out);
    void invert3x3(float *in, float *out) {
        out[0] = in[4]*in[8]-in[5]*in[7];
        out[3] = in[5]*in[6]-in[3]*in[8];
        out[6] = in[3]*in[7]-in[4]*in[6];

        float invdet = 1.0f/(in[0]*out[0] + in[1]*out[3] + in[2]*out[6]);

        out[0] *= invdet;
        out[3] *= invdet;
        out[6] *= invdet;

        out[1] = invdet*(in[7]*in[2]-in[8]*in[1]);
        out[4] = invdet*(in[8]*in[0]-in[6]*in[2]);
        out[7] = invdet*(in[6]*in[1]-in[7]*in[0]);
        
        out[2] = invdet*(in[1]*in[5]-in[2]*in[4]);
        out[5] = invdet*(in[2]*in[3]-in[0]*in[5]);
        out[8] = invdet*(in[0]*in[4]-in[1]*in[3]);

    }

    void invert3x3(double *in, double *out);
    void invert3x3(double *in, double *out) {
        out[0] = in[4]*in[8]-in[5]*in[7];
        out[3] = in[5]*in[6]-in[3]*in[8];
        out[6] = in[3]*in[7]-in[4]*in[6];

        double invdet = 1.0/(in[0]*out[0] + in[1]*out[3] + in[2]*out[6]);

        out[0] *= invdet;
        out[3] *= invdet;
        out[6] *= invdet;

        out[1] = invdet*(in[7]*in[2]-in[8]*in[1]);
        out[4] = invdet*(in[8]*in[0]-in[6]*in[2]);
        out[7] = invdet*(in[6]*in[1]-in[7]*in[0]);
        
        out[2] = invdet*(in[1]*in[5]-in[2]*in[4]);
        out[5] = invdet*(in[2]*in[3]-in[0]*in[5]);
        out[8] = invdet*(in[0]*in[4]-in[1]*in[3]);

    }

    void colorMatrixInterpolate(const float *a, const float *b, float alpha, float *result);
    void colorMatrixInterpolate(const float *a, const float *b, float alpha, float *result) {
        // Check for identity interpolations
        if (alpha == 0.f) {
            for (int i=0; i < 12; i++)  result[i] = a[i];
            return;
        } 
        if (alpha == 1.f) {
            for (int i=0; i < 12; i++)  result[i] = b[i];
            return;
        }
        // just do a linear interpolation between two known settings
        for (int i = 0; i < 12; i++) {
            result[i] = (1-alpha)*a[i] + alpha*b[i];
        }

        // rescale the result so that the minimum row sum is one
        float rowSum[3] = {result[0] + result[1] + result[2],
                           result[4] + result[5] + result[6],
                           result[8] + result[9] + result[10]};
        float scale = 1.0f;
        if (rowSum[0] < rowSum[1] && rowSum[0] < rowSum[2]) {
            scale = 1.0f/rowSum[0];
        } else if (rowSum[1] < rowSum[2]) {
            scale = 1.0f/rowSum[1];
        } else {
            scale = 1.0f/rowSum[2];
        }

        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 3; i++) {
                result[j*4+i] *= scale;
            }
        }                           

    }

}
