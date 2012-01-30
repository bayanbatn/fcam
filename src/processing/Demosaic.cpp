#include <cmath>
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <string.h>
#ifdef FCAM_ARCH_ARM
#include "Demosaic_ARM.h"
#endif

#include <FCam/processing/Demosaic.h>
#include <FCam/Sensor.h>
#include <FCam/Time.h>


namespace FCam {

    // Make a linear luminance -> pixel value lookup table
    void makeLUT(const Frame &f, float contrast, int blackLevel, float gamma, unsigned char *lut);
    void makeLUT(const Frame &f, float contrast, int blackLevel, float gamma, unsigned char *lut) {
        unsigned short minRaw = f.platform().minRawValue()+blackLevel;
        unsigned short maxRaw = f.platform().maxRawValue();
        
        for (int i = 0; i <= minRaw; i++) {
            lut[i] = 0;
        }
        
        float invRange = 1.0f/(maxRaw - minRaw);
        float b = 2 - powf(2.0f, contrast/100.0f);
        float a = 2 - 2*b; 
        for (int i = minRaw+1; i <= maxRaw; i++) {
            // Get a linear luminance in the range 0-1
            float y = (i-minRaw)*invRange;
            // Gamma correct it
            y = powf(y, 1.0f/gamma);
            // Apply a piecewise quadratic contrast curve
            if (y > 0.5) {
                y = 1-y;
                y = a*y*y + b*y;
                y = 1-y;
            } else {
                y = a*y*y + b*y;
            }
            // Convert to 8 bit and save
            y = std::floor(y * 255 + 0.5f);
            if (y < 0) y = 0;
            if (y > 255) y = 255;
            lut[i] = (unsigned char)y;
        }
        
        // add a guard band
        for (int i = maxRaw+1; i < 4096; i++) {
            lut[i] = 255;
        }
    }

    // Some functions used by demosaic
    inline short max(short a, short b) {return a>b ? a : b;}
    inline short max(short a, short b, short c, short d) {return max(max(a, b), max(c, d));}
    inline short min(short a, short b) {return a<b ? a : b;}

    Image demosaic(Frame src, float contrast, bool denoise, int blackLevel, float gamma) {
        if (!src.image().valid()) {
            error(Event::DemosaicError, "Cannot demosaic an invalid image");
            return Image();
        }
        if (src.image().bytesPerRow() % 2 == 1) {
            error(Event::DemosaicError, "Cannot demosaic an image with bytesPerRow not divisible by 2");
            return Image();
        }
       
        // We've vectorized this code for arm
        #ifdef FCAM_ARCH_ARM
        return demosaic_ARM(src, contrast, denoise, blackLevel, gamma);
        #endif

        Image input = src.image();

        // First check we're the right bayer pattern. If not crop and continue.
        switch((int)src.platform().bayerPattern()) {
        case GRBG:
            break;
        case RGGB:
            input = input.subImage(1, 0, Size(input.width()-2, input.height()));
            break;
        case BGGR:
            input = input.subImage(0, 1, Size(input.width(), input.height()-2));
            break;
        case GBRG:
            input = input.subImage(1, 1, Size(input.width()-2, input.height()-2));
        default:
            error(Event::DemosaicError, "Can't demosaic from a non-bayer sensor\n");
            return Image();
        }

        const int BLOCK_WIDTH = 40;
        const int BLOCK_HEIGHT = 24;
        const int G = 0, GR = 0, R = 1, B = 2, GB = 3;        

        int rawWidth = input.width();
        int rawHeight = input.height();
        int outWidth = rawWidth-8;
        int outHeight = rawHeight-8;
        outWidth /= BLOCK_WIDTH;
        outWidth *= BLOCK_WIDTH;
        outHeight /= BLOCK_HEIGHT;
        outHeight *= BLOCK_HEIGHT;

        Image out(outWidth, outHeight, RGB24);               

        // Check we're the right size, if not, crop center
        if (((input.width() - 8) != (unsigned)outWidth) ||
            ((input.height() - 8) != (unsigned)outHeight)) { 
            int offX = (input.width() - 8 - outWidth)/2;
            int offY = (input.height() - 8 - outHeight)/2;
            offX -= offX&1;
            offY -= offY&1;
            
            if (offX || offY) {
                input = input.subImage(offX, offY, Size(outWidth+8, outHeight+8));
            }
        }           

        // Prepare the lookup table
        unsigned char lut[4096];
        makeLUT(src, contrast, blackLevel, gamma, lut);

        // Grab the color matrix
        float colorMatrix[12];
        // Check if there's a custom color matrix
        if (src.shot().colorMatrix().size() == 12) {
            for (int i = 0; i < 12; i++) {
                colorMatrix[i] = src.shot().colorMatrix()[i];
            }
        } else {
            // Otherwise use the platform version
            src.platform().rawToRGBColorMatrix(src.shot().whiteBalance, colorMatrix);
        }

        for (int by = 0; by < rawHeight-8-BLOCK_HEIGHT+1; by += BLOCK_HEIGHT) {
            for (int bx = 0; bx < rawWidth-8-BLOCK_WIDTH+1; bx += BLOCK_WIDTH) {
                /*
                  Stage 1: Load a block of input, treat it as 4-channel gr, r, b, gb
                */
                short inBlock[4][BLOCK_HEIGHT/2+4][BLOCK_WIDTH/2+4];

                for (int y = 0; y < BLOCK_HEIGHT/2+4; y++) {
                    for (int x = 0; x < BLOCK_WIDTH/2+4; x++) {
                        inBlock[GR][y][x] = ((short *)((void *)input(bx + 2*x, by + 2*y)))[0];
                        inBlock[R][y][x] = ((short *)((void *)input(bx + 2*x+1, by + 2*y)))[0];
                        inBlock[B][y][x] = ((short *)((void *)input(bx + 2*x, by + 2*y+1)))[0];
                        inBlock[GB][y][x] = ((short *)((void *)input(bx + 2*x+1, by + 2*y+1)))[0];
                    }
                }

                // linear luminance outputs
                short linear[3][4][BLOCK_HEIGHT/2+4][BLOCK_WIDTH/2+4];

                /*                  

                Stage 1.5: Suppress hot pixels

                gr[HERE] = min(gr[HERE], max(gr[UP], gr[LEFT], gr[RIGHT], gr[DOWN]));
                r[HERE]  = min(r[HERE], max(r[UP], r[LEFT], r[RIGHT], r[DOWN]));
                b[HERE]  = min(b[HERE], max(b[UP], b[LEFT], b[RIGHT], b[DOWN]));
                gb[HERE] = min(gb[HERE], max(gb[UP], gb[LEFT], gb[RIGHT], gb[DOWN]));
 
                */

                if (denoise) {
                    for (int y = 1; y < BLOCK_HEIGHT/2+3; y++) {
                        for (int x = 1; x < BLOCK_WIDTH/2+3; x++) {
                            linear[G][GR][y][x] = min(inBlock[GR][y][x],
                                                      max(inBlock[GR][y-1][x],
                                                          inBlock[GR][y+1][x],
                                                          inBlock[GR][y][x+1],
                                                          inBlock[GR][y][x-1]));
                            linear[R][R][y][x] = min(inBlock[R][y][x],
                                                     max(inBlock[R][y-1][x],
                                                         inBlock[R][y+1][x],
                                                         inBlock[R][y][x+1],
                                                         inBlock[R][y][x-1]));
                            linear[B][B][y][x] = min(inBlock[B][y][x],
                                                     max(inBlock[B][y-1][x],
                                                         inBlock[B][y+1][x],
                                                         inBlock[B][y][x+1],
                                                         inBlock[B][y][x-1]));
                            linear[G][GB][y][x] = min(inBlock[GB][y][x],
                                                      max(inBlock[GB][y-1][x],
                                                          inBlock[GB][y+1][x],
                                                          inBlock[GB][y][x+1],
                                                          inBlock[GB][y][x-1]));
                        }
                    }
                } else {
                    for (int y = 1; y < BLOCK_HEIGHT/2+3; y++) {
                        for (int x = 1; x < BLOCK_WIDTH/2+3; x++) {
                            linear[G][GR][y][x] = inBlock[GR][y][x];
                            linear[R][R][y][x] = inBlock[R][y][x];
                            linear[B][B][y][x] = inBlock[B][y][x];
                            linear[G][GB][y][x] = inBlock[GB][y][x];
                        }
                    }                    
                }
                

                /*
                  2: Interpolate g at r 
                  
                  gv_r = (gb[UP] + gb[HERE])/2;
                  gvd_r = |gb[UP] - gb[HERE]|;
                  
                  gh_r = (gr[HERE] + gr[RIGHT])/2;
                  ghd_r = |gr[HERE] - gr[RIGHT]|;
                  
                  g_r = ghd_r < gvd_r ? gh_r : gv_r;
                  
                  3: Interpolate g at b
                  
                  gv_b = (gr[DOWN] + gr[HERE])/2;
                  gvd_b = |gr[DOWN] - gr[HERE]|;
                  
                  gh_b = (gb[LEFT] + gb[HERE])/2;
                  ghd_b = |gb[LEFT] - gb[HERE]|;
                  
                  g_b = ghd_b < gvd_b ? gh_b : gv_b;

                */

                for (int y = 1; y < BLOCK_HEIGHT/2+3; y++) {
                    for (int x = 1; x < BLOCK_WIDTH/2+3; x++) {
                        short gv_r = (linear[G][GB][y-1][x] + linear[G][GB][y][x])/2;
                        short gvd_r = abs(linear[G][GB][y-1][x] - linear[G][GB][y][x]);
                        short gh_r = (linear[G][GR][y][x] + linear[G][GR][y][x+1])/2;
                        short ghd_r = abs(linear[G][GR][y][x] - linear[G][GR][y][x+1]);
                        linear[G][R][y][x] = ghd_r < gvd_r ? gh_r : gv_r;

                        short gv_b = (linear[G][GR][y+1][x] + linear[G][GR][y][x])/2;
                        short gvd_b = abs(linear[G][GR][y+1][x] - linear[G][GR][y][x]);
                        short gh_b = (linear[G][GB][y][x] + linear[G][GB][y][x-1])/2;
                        short ghd_b = abs(linear[G][GB][y][x] - linear[G][GB][y][x-1]);
                        linear[G][B][y][x] = ghd_b < gvd_b ? gh_b : gv_b;                        
                    }
                }

                /*
                  4: Interpolate r at gr
                  
                  r_gr = (r[LEFT] + r[HERE])/2 + gr[HERE] - (g_r[LEFT] + g_r[HERE])/2;
                  
                  5: Interpolate b at gr
                  
                  b_gr = (b[UP] + b[HERE])/2 + gr[HERE] - (g_b[UP] + g_b[HERE])/2;
                  
                  6: Interpolate r at gb
                  
                  r_gb = (r[HERE] + r[DOWN])/2 + gb[HERE] - (g_r[HERE] + g_r[DOWN])/2;
                  
                  7: Interpolate b at gb
                  
                  b_gb = (b[HERE] + b[RIGHT])/2 + gb[HERE] - (g_b[HERE] + g_b[RIGHT])/2;
                */
                for (int y = 1; y < BLOCK_HEIGHT/2+3; y++) {
                    for (int x = 1; x < BLOCK_WIDTH/2+3; x++) {
                        linear[R][GR][y][x] = ((linear[R][R][y][x-1] + linear[R][R][y][x])/2 +
                                               linear[G][GR][y][x] - 
                                               (linear[G][R][y][x-1] + linear[G][R][y][x])/2);

                        linear[B][GR][y][x] = ((linear[B][B][y-1][x] + linear[B][B][y][x])/2 +
                                               linear[G][GR][y][x] - 
                                               (linear[G][B][y-1][x] + linear[G][B][y][x])/2);

                        linear[R][GB][y][x] = ((linear[R][R][y][x] + linear[R][R][y+1][x])/2 +
                                               linear[G][GB][y][x] - 
                                               (linear[G][R][y][x] + linear[G][R][y+1][x])/2);

                        linear[B][GB][y][x] = ((linear[B][B][y][x] + linear[B][B][y][x+1])/2 +
                                               linear[G][GB][y][x] - 
                                               (linear[G][B][y][x] + linear[G][B][y][x+1])/2);

                    }
                }       


                /*
                  
                8: Interpolate r at b
                
                rp_b = (r[DOWNLEFT] + r[HERE])/2 + g_b[HERE] - (g_r[DOWNLEFT] + g_r[HERE])/2;
                rn_b = (r[LEFT] + r[DOWN])/2 + g_b[HERE] - (g_r[LEFT] + g_r[DOWN])/2;
                rpd_b = (r[DOWNLEFT] - r[HERE]);
                rnd_b = (r[LEFT] - r[DOWN]);    
                
                r_b = rpd_b < rnd_b ? rp_b : rn_b;
                
                9: Interpolate b at r
                
                bp_r = (b[UPRIGHT] + b[HERE])/2 + g_r[HERE] - (g_b[UPRIGHT] + g_b[HERE])/2;
                bn_r = (b[RIGHT] + b[UP])/2 + g_r[HERE] - (g_b[RIGHT] + g_b[UP])/2;     
                bpd_r = |b[UPRIGHT] - b[HERE]|;
                bnd_r = |b[RIGHT] - b[UP]|;     
                
                b_r = bpd_r < bnd_r ? bp_r : bn_r;             
                
                */
                for (int y = 1; y < BLOCK_HEIGHT/2+3; y++) {
                    for (int x = 1; x < BLOCK_WIDTH/2+3; x++) {
                        short rp_b = ((linear[R][R][y+1][x-1] + linear[R][R][y][x])/2 +
                                      linear[G][B][y][x] - 
                                      (linear[G][R][y+1][x-1] + linear[G][R][y][x])/2);
                        short rpd_b = abs(linear[R][R][y+1][x-1] - linear[R][R][y][x]);
                        
                        short rn_b = ((linear[R][R][y][x-1] + linear[R][R][y+1][x])/2 +
                                      linear[G][B][y][x] - 
                                      (linear[G][R][y][x-1] + linear[G][R][y+1][x])/2);
                        short rnd_b = abs(linear[R][R][y][x-1] - linear[R][R][y+1][x]);
                        
                        linear[R][B][y][x] = rpd_b < rnd_b ? rp_b : rn_b;

                        short bp_r = ((linear[B][B][y-1][x+1] + linear[B][B][y][x])/2 +
                                      linear[G][R][y][x] - 
                                      (linear[G][B][y-1][x+1] + linear[G][B][y][x])/2);
                        short bpd_r = abs(linear[B][B][y-1][x+1] - linear[B][B][y][x]);
                        
                        short bn_r = ((linear[B][B][y][x+1] + linear[B][B][y-1][x])/2 +
                                      linear[G][R][y][x] - 
                                      (linear[G][B][y][x+1] + linear[G][B][y-1][x])/2);
                        short bnd_r = abs(linear[B][B][y][x+1] - linear[B][B][y-1][x]);
                        
                        linear[B][R][y][x] = bpd_r < bnd_r ? bp_r : bn_r;                       
                    }
                }

                /*
                  10: Color matrix
                    
                  11: Gamma correct
           
                */

                float r, g, b;
                unsigned short ri, gi, bi;
                for (int y = 2; y < BLOCK_HEIGHT/2+2; y++) {
                    for (int x = 2; x < BLOCK_WIDTH/2+2; x++) {

                        // Convert from sensor rgb to srgb
                        r = colorMatrix[0]*linear[R][GR][y][x] +
                            colorMatrix[1]*linear[G][GR][y][x] +
                            colorMatrix[2]*linear[B][GR][y][x] +
                            colorMatrix[3];

                        g = colorMatrix[4]*linear[R][GR][y][x] +
                            colorMatrix[5]*linear[G][GR][y][x] +
                            colorMatrix[6]*linear[B][GR][y][x] +
                            colorMatrix[7];

                        b = colorMatrix[8]*linear[R][GR][y][x] +
                            colorMatrix[9]*linear[G][GR][y][x] +
                            colorMatrix[10]*linear[B][GR][y][x] +
                            colorMatrix[11];

                        // Clamp
                        ri = r < 0 ? 0 : (r > 1023 ? 1023 : (unsigned short)(r+0.5f));
                        gi = g < 0 ? 0 : (g > 1023 ? 1023 : (unsigned short)(g+0.5f));
                        bi = b < 0 ? 0 : (b > 1023 ? 1023 : (unsigned short)(b+0.5f));
                       
                        // Gamma correct and store
                        out(bx+(x-2)*2, by+(y-2)*2)[0] = lut[ri];
                        out(bx+(x-2)*2, by+(y-2)*2)[1] = lut[gi];
                        out(bx+(x-2)*2, by+(y-2)*2)[2] = lut[bi];

                        // Convert from sensor rgb to srgb
                        r = colorMatrix[0]*linear[R][R][y][x] +
                            colorMatrix[1]*linear[G][R][y][x] +
                            colorMatrix[2]*linear[B][R][y][x] +
                            colorMatrix[3];

                        g = colorMatrix[4]*linear[R][R][y][x] +
                            colorMatrix[5]*linear[G][R][y][x] +
                            colorMatrix[6]*linear[B][R][y][x] +
                            colorMatrix[7];

                        b = colorMatrix[8]*linear[R][R][y][x] +
                            colorMatrix[9]*linear[G][R][y][x] +
                            colorMatrix[10]*linear[B][R][y][x] +
                            colorMatrix[11];

                        // Clamp
                        ri = r < 0 ? 0 : (r > 1023 ? 1023 : (unsigned short)(r+0.5f));
                        gi = g < 0 ? 0 : (g > 1023 ? 1023 : (unsigned short)(g+0.5f));
                        bi = b < 0 ? 0 : (b > 1023 ? 1023 : (unsigned short)(b+0.5f));
                        
                        // Gamma correct and store
                        out(bx+(x-2)*2+1, by+(y-2)*2)[0] = lut[ri];
                        out(bx+(x-2)*2+1, by+(y-2)*2)[1] = lut[gi];
                        out(bx+(x-2)*2+1, by+(y-2)*2)[2] = lut[bi];
                        
                        // Convert from sensor rgb to srgb
                        r = colorMatrix[0]*linear[R][B][y][x] +
                            colorMatrix[1]*linear[G][B][y][x] +
                            colorMatrix[2]*linear[B][B][y][x] +
                            colorMatrix[3];

                        g = colorMatrix[4]*linear[R][B][y][x] +
                            colorMatrix[5]*linear[G][B][y][x] +
                            colorMatrix[6]*linear[B][B][y][x] +
                            colorMatrix[7];

                        b = colorMatrix[8]*linear[R][B][y][x] +
                            colorMatrix[9]*linear[G][B][y][x] +
                            colorMatrix[10]*linear[B][B][y][x] +
                            colorMatrix[11];

                        // Clamp
                        ri = r < 0 ? 0 : (r > 1023 ? 1023 : (unsigned short)(r+0.5f));
                        gi = g < 0 ? 0 : (g > 1023 ? 1023 : (unsigned short)(g+0.5f));
                        bi = b < 0 ? 0 : (b > 1023 ? 1023 : (unsigned short)(b+0.5f));
                       
                        // Gamma correct and store
                        out(bx+(x-2)*2, by+(y-2)*2+1)[0] = lut[ri];
                        out(bx+(x-2)*2, by+(y-2)*2+1)[1] = lut[gi];
                        out(bx+(x-2)*2, by+(y-2)*2+1)[2] = lut[bi];
                        
                        // Convert from sensor rgb to srgb
                        r = colorMatrix[0]*linear[R][GB][y][x] +
                            colorMatrix[1]*linear[G][GB][y][x] +
                            colorMatrix[2]*linear[B][GB][y][x] +
                            colorMatrix[3];

                        g = colorMatrix[4]*linear[R][GB][y][x] +
                            colorMatrix[5]*linear[G][GB][y][x] +
                            colorMatrix[6]*linear[B][GB][y][x] +
                            colorMatrix[7];

                        b = colorMatrix[8]*linear[R][GB][y][x] +
                            colorMatrix[9]*linear[G][GB][y][x] +
                            colorMatrix[10]*linear[B][GB][y][x] +
                            colorMatrix[11];

                        // Clamp
                        ri = r < 0 ? 0 : (r > 1023 ? 1023 : (unsigned short)(r+0.5f));
                        gi = g < 0 ? 0 : (g > 1023 ? 1023 : (unsigned short)(g+0.5f));
                        bi = b < 0 ? 0 : (b > 1023 ? 1023 : (unsigned short)(b+0.5f));
                       
                        // Gamma correct and store
                        out(bx+(x-2)*2+1, by+(y-2)*2+1)[0] = lut[ri];
                        out(bx+(x-2)*2+1, by+(y-2)*2+1)[1] = lut[gi];
                        out(bx+(x-2)*2+1, by+(y-2)*2+1)[2] = lut[bi];
                        
                    }
                }                
            }
        }

        return out;
    }

    // Generic RAW to thumbnail converter
    Image makeThumbnailRAW(Frame src, const Size &thumbSize, float contrast, int blackLevel, float gamma);
    Image makeThumbnailRAW(Frame src, const Size &thumbSize, float contrast, int blackLevel, float gamma) {
        
        // Special-case the N900's common case for speed (5 MP GRGB -> 640x480 RGB24 on ARM)
        #ifdef FCAM_ARCH_ARM
        if (src.image().width() == 2592 &&
            src.image().height() == 1968 &&
            thumbSize.width == 640 &&
            thumbSize.height == 480 &&
            src.platform().bayerPattern() == GRBG) {
            return makeThumbnailRAW_ARM(src, contrast, blackLevel, gamma);
        }
        #endif

        // Make the response curve
        unsigned char lut[4096];
        makeLUT(src, contrast, blackLevel, gamma, lut);

        Image thumb;
        //!TODO: Add thumbnailing support for other Bayer patterns 
        bool redRowEven, blueRowGreenPixelEven;
        switch (src.platform().bayerPattern()) {
        case RGGB:
            redRowEven = true;
            blueRowGreenPixelEven = true;
            break;
        case BGGR:
            redRowEven = false;
            blueRowGreenPixelEven = false;
            break;
        case GRBG:
            redRowEven = true;
            blueRowGreenPixelEven = false;
            break;
        case GBRG:
            redRowEven = false;
            blueRowGreenPixelEven = true;
            break;
        default:
            return thumb;
            break;
        }

        Time startTime = Time::now();

        thumb = Image(thumbSize, RGB24);

        unsigned int w = src.image().width();
        unsigned int h = src.image().height();
        unsigned int tw = thumbSize.width;
        unsigned int th = thumbSize.height;
        short maxPixel = 1023;
        short minPixel = 0;
        unsigned int scaleX = (int)std::floor((float)w / tw);
        unsigned int scaleY = (int)std::floor((float)h / th);
        unsigned int scale = std::min(scaleX, scaleY); // Maintain aspect ratio
        
        int cropX = (w-scale*tw)/2;
        if (cropX % 2 == 1) cropX--; // Ensure we're at start of 2x2 block
        int cropY = (h-scale*th)/2;
        if (cropY % 2 == 1) cropY--; // Ensure we're at start of 2x2 block

        float colorMatrix[12];
        // Check if there's a custom color matrix
        if (src.shot().colorMatrix().size() == 12) {
            for (int i = 0; i < 12; i++) {
                colorMatrix[i] = src.shot().colorMatrix()[i];
            }
        } else {
            // Otherwise use the platform version
            src.platform().rawToRGBColorMatrix(src.shot().whiteBalance, colorMatrix);
        }

        /* A fast downsampling/demosaicing - average down color
           channels over the whole source pixel block under a single
           thumbnail pixel, and just use those colors directly as the
           pixel colors. */        

        for (unsigned int ty=0; ty < th; ty++) {
            unsigned char *tpix = thumb(0,ty); // Get a pointer to the beginning of the row
            for (unsigned int tx=0; tx < tw; tx++) {
                unsigned int r=0,g=0,b=0;
                unsigned int rc=0,gc=0,bc=0;

                unsigned char *rowStart = src.image()(cropX + tx*scale, cropY + ty*scale);

                bool isRedRow = ((ty*scale % 2 == 0) == redRowEven);

                for(unsigned int i=0; i<scale; i++, rowStart+=src.image().bytesPerRow(), isRedRow = !isRedRow) {                    
                    unsigned short *px = (unsigned short *)((void *)rowStart);
                    if (isRedRow) {
                        bool isRed=((tx*scale)%2==0) == blueRowGreenPixelEven;
                        for (unsigned int j=0; j < scale; j++, isRed=!isRed) {
                            if (isRed) {
                                r += *(px++);
                                rc++;
                            } else {
                                g += *(px++);
                                gc++;
                            }
                        }
                    } else {
                        bool isGreen=((tx*scale)%2==0) == blueRowGreenPixelEven;
                        for (unsigned int j=0; j < scale; j++, isGreen=!isGreen) {
                            if (isGreen) {
                                g += *(px++);
                                gc++;
                            } else {
                                b += *(px++);
                                bc++;
                            }
                            
                        }
                    }
                }
                float r_sensor = ((float)r / rc);
                float g_sensor = ((float)g / gc);
                float b_sensor = ((float)b / bc);
                float r_srgb = (r_sensor * colorMatrix[0] +
                                g_sensor * colorMatrix[1] +
                                b_sensor * colorMatrix[2] +
                                colorMatrix[3]);
                float g_srgb = (r_sensor * colorMatrix[4] +
                                g_sensor * colorMatrix[5] +
                                b_sensor * colorMatrix[6] +
                                colorMatrix[7]);
                float b_srgb = (r_sensor * colorMatrix[8] +
                                g_sensor * colorMatrix[9] +
                                b_sensor * colorMatrix[10] +
                                colorMatrix[11]);
                unsigned short r_linear = std::min(maxPixel,std::max(minPixel,(short int)std::floor(r_srgb+0.5)));
                unsigned short g_linear = std::min(maxPixel,std::max(minPixel,(short int)std::floor(g_srgb+0.5)));
                unsigned short b_linear = std::min(maxPixel,std::max(minPixel,(short int)std::floor(b_srgb+0.5)));
                *(tpix++) = lut[r_linear];
                *(tpix++) = lut[g_linear];
                *(tpix++) = lut[b_linear];
            }
        }

        //std::cout << "Done creating thumbnail. time = " << ((Time::now()-startTime)/1000) << std::endl;

        return thumb;

    }

    Image makeThumbnail(Frame src, const Size &thumbSize, float contrast, int blackLevel, float gamma) {
        Image thumb;

        // Sanity checks
        if (not src.image().valid()) return thumb;
        if (thumbSize.width == 0 or thumbSize.height == 0) return thumb;

        switch (src.image().type()) {
        case RAW:
            thumb = makeThumbnailRAW(src, thumbSize, contrast, blackLevel, gamma);
            break;
        case RGB24:
        case RGB16:
        case UYVY:
        case YUV24:
        case UNKNOWN:
        default:
            //!TODO: Add thumbnailing support for other formats
            // These formats can't be thumbnailed yet
            break;
        }
        return thumb;
    }

}
