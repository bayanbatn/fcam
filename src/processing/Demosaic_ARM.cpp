#ifdef FCAM_ARCH_ARM
#include "Demosaic_ARM.h"
#include <arm_neon.h>

namespace FCam {

    // Make a linear luminance -> pixel value lookup table
    extern void makeLUT(const Frame &f, float contrast, int blackLevel, float gamma, unsigned char *lut);

    Image demosaic_ARM(Frame src, float contrast, bool denoise, int blackLevel, float gamma) {

        const int BLOCK_WIDTH  = 40;
        const int BLOCK_HEIGHT = 24;

        Image input = src.image();

        // Check we're the right bayer pattern. If not crop and continue.
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

        int rawWidth = input.width();
        int rawHeight = input.height();

        const int VEC_WIDTH = ((BLOCK_WIDTH + 8)/8);
        const int VEC_HEIGHT = ((BLOCK_HEIGHT + 8)/2);       

        int rawPixelsPerRow = input.bytesPerRow()/2 ; // Assumes bytesPerRow is even

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
        
        Time startTime = Time::now(); 

        // Prepare the color matrix in S8.8 fixed point
        float colorMatrix_f[12];
        
        // Check if there's a custom color matrix
        if (src.shot().colorMatrix().size() == 12) {
            for (int i = 0; i < 12; i++) {
                colorMatrix_f[i] = src.shot().colorMatrix()[i];
            }
        } else {
            // Otherwise use the platform version
            src.platform().rawToRGBColorMatrix(src.shot().whiteBalance, colorMatrix_f);
        }

        int16x4_t colorMatrix[3];
        for (int i = 0; i < 3; i++) {
            int16_t val = (int16_t)(colorMatrix_f[i*4+0] * 256 + 0.5);
            colorMatrix[i] = vld1_lane_s16(&val, colorMatrix[i], 0);
            val = (int16_t)(colorMatrix_f[i*4+1] * 256 + 0.5);
            colorMatrix[i] = vld1_lane_s16(&val, colorMatrix[i], 1);
            val = (int16_t)(colorMatrix_f[i*4+2] * 256 + 0.5);
            colorMatrix[i] = vld1_lane_s16(&val, colorMatrix[i], 2);
            val = (int16_t)(colorMatrix_f[i*4+3] * 256 + 0.5);
            colorMatrix[i] = vld1_lane_s16(&val, colorMatrix[i], 3);
        }

        // A buffer to store data after demosiac and color correction
        // but before gamma correction
        uint16_t out16[BLOCK_WIDTH*BLOCK_HEIGHT*3];

        // Various color channels. Only 4 of them are defined before
        // demosaic, all of them are defined after demosiac
        int16_t scratch[VEC_WIDTH*VEC_HEIGHT*4*12];

        #define R_R_OFF  (VEC_WIDTH*VEC_HEIGHT*4*0)
        #define R_GR_OFF (VEC_WIDTH*VEC_HEIGHT*4*1)
        #define R_GB_OFF (VEC_WIDTH*VEC_HEIGHT*4*2)
        #define R_B_OFF  (VEC_WIDTH*VEC_HEIGHT*4*3)

        #define G_R_OFF  (VEC_WIDTH*VEC_HEIGHT*4*4)
        #define G_GR_OFF (VEC_WIDTH*VEC_HEIGHT*4*5)
        #define G_GB_OFF (VEC_WIDTH*VEC_HEIGHT*4*6)
        #define G_B_OFF  (VEC_WIDTH*VEC_HEIGHT*4*7)

        #define B_R_OFF  (VEC_WIDTH*VEC_HEIGHT*4*8)
        #define B_GR_OFF (VEC_WIDTH*VEC_HEIGHT*4*9)
        #define B_GB_OFF (VEC_WIDTH*VEC_HEIGHT*4*10)
        #define B_B_OFF  (VEC_WIDTH*VEC_HEIGHT*4*11)

        #define R_R(i)  (scratch+(i)+R_R_OFF)
        #define R_GR(i) (scratch+(i)+R_GR_OFF)
        #define R_GB(i) (scratch+(i)+R_GB_OFF)
        #define R_B(i)  (scratch+(i)+R_B_OFF)

        #define G_R(i)  (scratch+(i)+G_R_OFF)
        #define G_GR(i) (scratch+(i)+G_GR_OFF)
        #define G_GB(i) (scratch+(i)+G_GB_OFF)
        #define G_B(i)  (scratch+(i)+G_B_OFF)

        #define B_R(i)  (scratch+(i)+B_R_OFF)
        #define B_GR(i) (scratch+(i)+B_GR_OFF)
        #define B_GB(i) (scratch+(i)+B_GB_OFF)
        #define B_B(i)  (scratch+(i)+B_B_OFF)

        // Reuse some of the output scratch area for the noisy inputs
        #define G_GR_NOISY B_GR
        #define B_B_NOISY  G_B
        #define R_R_NOISY  G_R
        #define G_GB_NOISY B_GB

        // Prepare the lookup table
        unsigned char lut[4096];
        makeLUT(src, contrast, blackLevel, gamma, lut);

        // For each block in the input
        for (int by = 0; by < rawHeight-8-BLOCK_HEIGHT+1; by += BLOCK_HEIGHT) {
            const short * __restrict__ blockPtr = (const short *)input(0,by);
            unsigned char * __restrict__ outBlockPtr = out(0, by);
            for (int bx = 0; bx < rawWidth-8-BLOCK_WIDTH+1; bx += BLOCK_WIDTH) {                

                // Stage 1) Demux a block of input into L1
                if (1) {
                    register const int16_t * __restrict__ rawPtr = blockPtr;
                    register const int16_t * __restrict__ rawPtr2 = blockPtr + rawPixelsPerRow;

                    register const int rawJump = rawPixelsPerRow*2 - VEC_WIDTH*8;

                    register int16_t * __restrict__ g_gr_ptr = denoise ? G_GR_NOISY(0) : G_GR(0);
                    register int16_t * __restrict__ r_r_ptr  = denoise ? R_R_NOISY(0)  : R_R(0);
                    register int16_t * __restrict__ b_b_ptr  = denoise ? B_B_NOISY(0)  : B_B(0);
                    register int16_t * __restrict__ g_gb_ptr = denoise ? G_GB_NOISY(0) : G_GB(0);

                    for (int y = 0; y < VEC_HEIGHT; y++) {
                        for (int x = 0; x < VEC_WIDTH/2; x++) {

                            asm volatile ("# Stage 1) Demux\n");
                            
                            // The below needs to be volatile, but
                            // it's not clear why (if it's not, it
                            // gets optimized out entirely)
                            asm volatile (
                                 "vld2.16  {d6-d9}, [%[rawPtr]]! \n\t"
                                 "vld2.16  {d10-d13}, [%[rawPtr2]]! \n\t"
                                 "vst1.16  {d6-d7}, [%[g_gr_ptr]]! \n\t"
                                 "vst1.16  {d8-d9}, [%[r_r_ptr]]! \n\t"
                                 "vst1.16  {d10-d11}, [%[b_b_ptr]]! \n\t"
                                 "vst1.16  {d12-d13}, [%[g_gb_ptr]]! \n\t" :
                                 [rawPtr]"+r"(rawPtr), 
                                 [rawPtr2]"+r"(rawPtr2),
                                 [g_gr_ptr]"+r"(g_gr_ptr),
                                 [r_r_ptr]"+r"(r_r_ptr),
                                 [b_b_ptr]"+r"(b_b_ptr),
                                 [g_gb_ptr]"+r"(g_gb_ptr) ::
                                 "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "memory");
                            
                        }

                        rawPtr += rawJump;
                        rawPtr2 += rawJump;
                    }               
                }

                // Stage 1.5) Denoise sensor input (noisy pixel supression)

                // A pixel can't be brighter than its brightest neighbor

                if (denoise) {
                    register int16_t * __restrict__ ptr_in = NULL;
                    register int16_t * __restrict__ ptr_out = NULL;
                    asm volatile("#Stage 1.5: Denoise\n\t");
                    for (int b=0; b<4; b++) {
                        if (b==0) ptr_in = G_GR_NOISY(0);
                        if (b==1) ptr_in = R_R_NOISY(0);
                        if (b==2) ptr_in = B_B_NOISY(0);
                        if (b==3) ptr_in = G_GB_NOISY(0);
                        if (b==0) ptr_out = G_GR(0);
                        if (b==1) ptr_out = R_R(0);
                        if (b==2) ptr_out = B_B(0);
                        if (b==3) ptr_out = G_GB(0);

                        // write the top block pixels who aren't being denoised
                        for (int x = 0; x < (BLOCK_WIDTH+8); x+=8) {
                            int16x8_t in = vld1q_s16(ptr_in);
                            vst1q_s16(ptr_out, in);
                            ptr_in+=8;
                            ptr_out+=8;
                        }

                        for (int y = 1; y < VEC_HEIGHT - 1; y++) {
                            for (int x = 0; x < VEC_WIDTH/2; x++) {
                                int16x8_t here  = vld1q_s16(ptr_in);
                                int16x8_t above = vld1q_s16(ptr_in + VEC_WIDTH*4);
                                int16x8_t under = vld1q_s16(ptr_in - VEC_WIDTH*4);
                                int16x8_t right = vld1q_s16(ptr_in + 1);
                                int16x8_t left  = vld1q_s16(ptr_in - 1);
                                int16x8_t max, min;

                                // find the max and min of the neighbors
                                max = vmaxq_s16(left, right);
                                max = vmaxq_s16(above, max);
                                max = vmaxq_s16(under, max);

                                min = vminq_s16(left, right);
                                min = vminq_s16(above, min);
                                min = vminq_s16(under, min);                               

                                // clamp here to be within this range
                                here  = vminq_s16(max, here);
                                here  = vmaxq_s16(min, here);

                                vst1q_s16(ptr_out, here);
                                ptr_in += 8;
                                ptr_out += 8;
                            }
                        }

                        // write the bottom block pixels who aren't being denoised
                        for (int x = 0; x < (BLOCK_WIDTH+8); x+=8) {
                            int16x8_t in = vld1q_s16(ptr_in);
                            vst1q_s16(ptr_out, in);
                            ptr_in+=8;
                            ptr_out+=8;
                        }
                    }
                }

                // Stage 2 and 3) Do horizontal and vertical
                // interpolation of green, as well as picking the
                // output for green
                /*
                  gv_r = (gb[UP] + gb[HERE])/2;
                  gvd_r = (gb[UP] - gb[HERE]);
                  gh_r = (gr[HERE] + gr[RIGHT])/2;
                  ghd_r = (gr[HERE] - gr[RIGHT]);                 
                  g_r = ghd_r < gvd_r ? gh_r : gv_r;
                  
                  gv_b = (gr[DOWN] + gr[HERE])/2;
                  gvd_b = (gr[DOWN] - gr[HERE]);                  
                  gh_b = (gb[LEFT] + gb[HERE])/2;
                  ghd_b = (gb[LEFT] - gb[HERE]);                  
                  g_b = ghd_b < gvd_b ? gh_b : gv_b;
                */
                if (1) {
                
                    int i = VEC_WIDTH*4;

                    register int16_t *g_gb_up_ptr = G_GB(i) - VEC_WIDTH*4;
                    register int16_t *g_gb_here_ptr = G_GB(i);
                    register int16_t *g_gb_left_ptr = G_GB(i) - 1;
                    register int16_t *g_gr_down_ptr = G_GR(i) + VEC_WIDTH*4;
                    register int16_t *g_gr_here_ptr = G_GR(i);
                    register int16_t *g_gr_right_ptr = G_GR(i) + 1;
                    register int16_t *g_r_ptr = G_R(i);
                    register int16_t *g_b_ptr = G_B(i);
            
                    for (int y = 1; y < VEC_HEIGHT-1; y++) {
                        for (int x = 0; x < VEC_WIDTH/2; x++) {

                            asm volatile ("#Stage 2) Green interpolation\n");

                            // Load the inputs

                            int16x8_t gb_up = vld1q_s16(g_gb_up_ptr);
                            g_gb_up_ptr+=8;
                            int16x8_t gb_here = vld1q_s16(g_gb_here_ptr);
                            g_gb_here_ptr+=8;
                            int16x8_t gb_left = vld1q_s16(g_gb_left_ptr); // unaligned
                            g_gb_left_ptr+=8;
                            int16x8_t gr_down = vld1q_s16(g_gr_down_ptr);
                            g_gr_down_ptr+=8;
                            int16x8_t gr_here = vld1q_s16(g_gr_here_ptr);
                            g_gr_here_ptr+=8;
                            int16x8_t gr_right = vld1q_s16(g_gr_right_ptr); // unaligned
                            g_gr_right_ptr+=8;
                            
                            //I couldn't get this assembly to work, and I don't know which
                            // of the three blocks of assembly is wrong
                            // This asm should load the inputs
                            /*
                            asm volatile(
                            "vld1.16        {d16-d17}, [%[gb_up]]!\n\t"
                            "vld1.16        {d18-d19}, [%[gb_here]]!\n\t"
                            "vld1.16        {d20-d21}, [%[gb_left]]!\n\t"
                            "vld1.16        {d22-d23}, [%[gr_down]]!\n\t"
                            "vld1.16        {d24-d25}, [%[gr_here]]!\n\t"
                            "vld1.16        {d26-d27}, [%[gr_right]]!\n\t"
                            :
                            [gb_up]"+r"(g_gb_up_ptr),
                            [gb_here]"+r"(g_gb_here_ptr),
                            [gb_left]"+r"(g_gb_left_ptr),
                            [gr_down]"+r"(g_gr_down_ptr),
                            [gr_here]"+r"(g_gr_here_ptr),
                            [gr_right]"+r"(g_gr_right_ptr) :: 
                            //"d16","d17","d18","d19","d20","d21","d22","d23","d24","d25","d26","d27",
                            "q8","q9","q10","q11","q12","q13");

                            //q8 - gb_up
                            //q9 - gb_here
                            //q10 - gb_left
                            //q11 - gr_down
                            //q12 - gr_here
                            //q13 - gr_right
                            */

                            // Do the processing
                            int16x8_t gv_r  = vhaddq_s16(gb_up, gb_here);
                            int16x8_t gvd_r = vabdq_s16(gb_up, gb_here);
                            int16x8_t gh_r  = vhaddq_s16(gr_right, gr_here);
                            int16x8_t ghd_r = vabdq_s16(gr_here, gr_right);
                            int16x8_t g_r = vbslq_s16(vcltq_s16(ghd_r, gvd_r), gh_r, gv_r);

                            int16x8_t gv_b  = vhaddq_s16(gr_down, gr_here);
                            int16x8_t gvd_b = vabdq_s16(gr_down, gr_here);
                            int16x8_t gh_b  = vhaddq_s16(gb_left, gb_here);
                            int16x8_t ghd_b = vabdq_s16(gb_left, gb_here);
                            int16x8_t g_b = vbslq_s16(vcltq_s16(ghd_b, gvd_b), gh_b, gv_b);
                            
                            //this asm should do the above selection/interpolation
                            /*
                            asm volatile(
                            "vabd.s16       q0, q12, q13\n\t" //ghd_r
                            "vabd.s16       q1, q8, q9\n\t" //gvd_r
                            "vabd.s16       q2, q10, q9\n\t" //ghd_b
                            "vabd.s16       q3, q11, q12\n\t" //gvd_b
                            "vcgt.s16       q1, q0, q1\n\t" //select ghd_r or gvd_r
                            "vcgt.s16       q2, q2, q3\n\t" //select gvd_b or ghd_b
                            "vhadd.s16      q8, q8, q9\n\t" //gv_r
                            "vhadd.s16      q11, q11, q12\n\t" //gv_b
                            "vhadd.s16      q12, q12, q13\n\t" //gh_r
                            "vhadd.s16      q9, q9, q10\n\t" //gh_b
                            "vbsl           q1, q12, q8\n\t" //g_r
                            "vbsl           q2, q9, q11\n\t" //g_b
                             ::: "q0","q1","q2","q3","q8","q9","q10","q11","q12","q13");
                            */

                            //this should save the output
                            /*
                            asm volatile(
                            "vst1.16        {d2-d3}, [%[g_r]]!\n\t"
                            "vst1.16        {d4-d5}, [%[g_b]]!\n\t" :
                            [g_r]"+r"(g_r_ptr),[g_b]"+r"(g_b_ptr)
                            :: "memory");
                            */
                            
                            // Save the outputs
                            vst1q_s16(g_r_ptr, g_r);
                            g_r_ptr+=8;
                            vst1q_s16(g_b_ptr, g_b);
                            g_b_ptr+=8;
                        }
                    }
                }
                asm volatile ("#End of stage 2 (green interpolation)\n");
                // Stages 4-9

                if (1) {
                    
                    /*
                      r_gr = (r[LEFT] + r[HERE])/2 + gr[HERE] - (g_r[LEFT] + g_r[HERE])/2;
                      b_gr = (b[UP] + b[HERE])/2 + gr[HERE] - (g_b[UP] + g_b[HERE])/2;
                      r_gb = (r[HERE] + r[DOWN])/2 + gb[HERE] - (g_r[HERE] + g_r[DOWN])/2;
                      b_gb = (b[HERE] + b[RIGHT])/2 + gb[HERE] - (g_b[HERE] + g_b[RIGHT])/2;
                      
                      rp_b = (r[DOWNLEFT] + r[HERE])/2 + g_b[HERE] - (g_r[DOWNLEFT] + g_r[HERE])/2;
                      rn_b = (r[LEFT] + r[DOWN])/2 + g_b[HERE] - (g_r[LEFT] + g_r[DOWN])/2;
                      rpd_b = (r[DOWNLEFT] - r[HERE]);
                      rnd_b = (r[LEFT] - r[DOWN]);      
                      r_b = rpd_b < rnd_b ? rp_b : rn_b;                      
        
                      bp_r = (b[UPRIGHT] + b[HERE])/2 + g_r[HERE] - (g_b[UPRIGHT] + g_b[HERE])/2;
                      bn_r = (b[RIGHT] + b[UP])/2 + g_r[HERE] - (g_b[RIGHT] + g_b[UP])/2;       
                      bpd_r = (b[UPRIGHT] - b[HERE]);
                      bnd_r = (b[RIGHT] - b[UP]);       
                      b_r = bpd_r < bnd_r ? bp_r : bn_r;
                    */

                    int i = 2*VEC_WIDTH*4;

                    for (int y = 2; y < VEC_HEIGHT-2; y++) {
                        for (int x = 0; x < VEC_WIDTH; x++) {

                            asm volatile ("#Stage 4) r/b interpolation\n");

                            // Load the inputs
                            int16x4_t r_here       = vld1_s16(R_R(i));
                            int16x4_t r_left       = vld1_s16(R_R(i) - 1);
                            int16x4_t r_down       = vld1_s16(R_R(i) + VEC_WIDTH*4);

                            int16x4_t g_r_left     = vld1_s16(G_R(i) - 1);
                            int16x4_t g_r_here     = vld1_s16(G_R(i));
                            int16x4_t g_r_down     = vld1_s16(G_R(i) + VEC_WIDTH*4);

                            int16x4_t b_up         = vld1_s16(B_B(i) - VEC_WIDTH*4);
                            int16x4_t b_here       = vld1_s16(B_B(i));
                            int16x4_t b_right      = vld1_s16(B_B(i) + 1);

                            int16x4_t g_b_up       = vld1_s16(G_B(i) - VEC_WIDTH*4);
                            int16x4_t g_b_here     = vld1_s16(G_B(i));
                            int16x4_t g_b_right    = vld1_s16(G_B(i) + 1);

                            // Do the processing
                            int16x4_t gr_here      = vld1_s16(G_GR(i));
                            int16x4_t gb_here      = vld1_s16(G_GB(i));

                            { // red at green
                                int16x4_t r_gr  = vadd_s16(vhadd_s16(r_left, r_here),
                                                            vsub_s16(gr_here,
                                                                      vhadd_s16(g_r_left, g_r_here)));
                                int16x4_t r_gb  = vadd_s16(vhadd_s16(r_here, r_down),
                                                            vsub_s16(gb_here, 
                                                                      vhadd_s16(g_r_down, g_r_here)));
                                vst1_s16(R_GR(i), r_gr);
                                vst1_s16(R_GB(i), r_gb);
                            }
                            
                            { // red at blue
                                int16x4_t r_downleft   = vld1_s16(R_R(i) + VEC_WIDTH*4 - 1);
                                int16x4_t g_r_downleft = vld1_s16(G_R(i) + VEC_WIDTH*4 - 1);
                                
                                int16x4_t rp_b  = vadd_s16(vhadd_s16(r_downleft, r_here),
                                                            vsub_s16(g_b_here,
                                                                     vhadd_s16(g_r_downleft, g_r_here)));
                                int16x4_t rn_b  = vadd_s16(vhadd_s16(r_left, r_down),
                                                            vsub_s16(g_b_here,
                                                                     vhadd_s16(g_r_left, g_r_down)));
                                int16x4_t rpd_b = vabd_s16(r_downleft, r_here);
                                int16x4_t rnd_b = vabd_s16(r_left, r_down);
                                int16x4_t r_b   = vbsl_s16(vclt_s16(rpd_b, rnd_b), rp_b, rn_b);
                                vst1_s16(R_B(i), r_b);
                            }
                            
                            { // blue at green
                                int16x4_t b_gr  = vadd_s16(vhadd_s16(b_up, b_here),
                                                            vsub_s16(gr_here,
                                                                     vhadd_s16(g_b_up, g_b_here)));
                                int16x4_t b_gb  = vadd_s16(vhadd_s16(b_here, b_right),
                                                            vsub_s16(gb_here,
                                                                     vhadd_s16(g_b_right, g_b_here)));
                                vst1_s16(B_GR(i), b_gr);
                                vst1_s16(B_GB(i), b_gb);
                            }
                            
                            { // blue at red
                                int16x4_t b_upright    = vld1_s16(B_B(i) - VEC_WIDTH*4 + 1);
                                int16x4_t g_b_upright  = vld1_s16(G_B(i) - VEC_WIDTH*4 + 1);
                                
                                int16x4_t bp_r  = vadd_s16(vhadd_s16(b_upright, b_here),
                                                            vsub_s16(g_r_here, 
                                                                      vhadd_s16(g_b_upright, g_b_here)));
                                int16x4_t bn_r  = vadd_s16(vhadd_s16(b_right, b_up),
                                                            vsub_s16(g_r_here,
                                                                      vhadd_s16(g_b_right, g_b_up)));
                                int16x4_t bpd_r = vabd_s16(b_upright, b_here);
                                int16x4_t bnd_r = vabd_s16(b_right, b_up);
                                int16x4_t b_r   = vbsl_s16(vclt_s16(bpd_r, bnd_r), bp_r, bn_r);
                                vst1_s16(B_R(i), b_r);
                            }
                            
                            // Advance the index
                            i += 4;
                        }
                    }
                    asm volatile ("#End of stage 4 - what_ever\n\t");
                }

                // Stage 10)
                if (1) {
                    // Color-correct and save the results into a 16-bit buffer for gamma correction

                    asm volatile ("#Stage 10) Color Correction\n");

                    uint16_t * __restrict__ out16Ptr = out16;

                    int i = 2*VEC_WIDTH*4;

                    const uint16x4_t bound = vdup_n_u16(1023);

                    for (int y = 2; y < VEC_HEIGHT-2; y++) {

                        // skip the first vec in each row
                        
                        int16x4x2_t r0 = vzip_s16(vld1_s16(R_GR(i)), vld1_s16(R_R(i)));
                        int16x4x2_t g0 = vzip_s16(vld1_s16(G_GR(i)), vld1_s16(G_R(i)));
                        int16x4x2_t b0 = vzip_s16(vld1_s16(B_GR(i)), vld1_s16(B_R(i)));
                        i += 4;

                        for (int x = 1; x < VEC_WIDTH; x++) {
                            
                            int16x4x2_t r1 = vzip_s16(vld1_s16(R_GR(i)), vld1_s16(R_R(i)));
                            int16x4x2_t g1 = vzip_s16(vld1_s16(G_GR(i)), vld1_s16(G_R(i)));
                            int16x4x2_t b1 = vzip_s16(vld1_s16(B_GR(i)), vld1_s16(B_R(i)));
                            
                            // do the color matrix
                            int32x4_t rout = vmovl_s16(vdup_lane_s16(colorMatrix[0], 3));                       
                            rout = vmlal_lane_s16(rout, r0.val[1], colorMatrix[0], 0);
                            rout = vmlal_lane_s16(rout, g0.val[1], colorMatrix[0], 1);
                            rout = vmlal_lane_s16(rout, b0.val[1], colorMatrix[0], 2);
                            
                            int32x4_t gout = vmovl_s16(vdup_lane_s16(colorMatrix[1], 3));                       
                            gout = vmlal_lane_s16(gout, r0.val[1], colorMatrix[1], 0);
                            gout = vmlal_lane_s16(gout, g0.val[1], colorMatrix[1], 1);
                            gout = vmlal_lane_s16(gout, b0.val[1], colorMatrix[1], 2);
                            
                            int32x4_t bout = vmovl_s16(vdup_lane_s16(colorMatrix[2], 3));
                            bout = vmlal_lane_s16(bout, r0.val[1], colorMatrix[2], 0);
                            bout = vmlal_lane_s16(bout, g0.val[1], colorMatrix[2], 1);
                            bout = vmlal_lane_s16(bout, b0.val[1], colorMatrix[2], 2);
                            
                            uint16x4x3_t col16;
                            col16.val[0] = vqrshrun_n_s32(rout, 8);
                            col16.val[1] = vqrshrun_n_s32(gout, 8);
                            col16.val[2] = vqrshrun_n_s32(bout, 8);                     
                            col16.val[0] = vmin_u16(col16.val[0], bound);
                            col16.val[1] = vmin_u16(col16.val[1], bound);
                            col16.val[2] = vmin_u16(col16.val[2], bound);
                            vst3_u16(out16Ptr, col16);
                            out16Ptr += 12;
                            
                            rout = vmovl_s16(vdup_lane_s16(colorMatrix[0], 3));                 
                            rout = vmlal_lane_s16(rout, r1.val[0], colorMatrix[0], 0);
                            rout = vmlal_lane_s16(rout, g1.val[0], colorMatrix[0], 1);
                            rout = vmlal_lane_s16(rout, b1.val[0], colorMatrix[0], 2);
                            
                            gout = vmovl_s16(vdup_lane_s16(colorMatrix[1], 3));                 
                            gout = vmlal_lane_s16(gout, r1.val[0], colorMatrix[1], 0);
                            gout = vmlal_lane_s16(gout, g1.val[0], colorMatrix[1], 1);
                            gout = vmlal_lane_s16(gout, b1.val[0], colorMatrix[1], 2);
                            
                            bout = vmovl_s16(vdup_lane_s16(colorMatrix[2], 3));
                            bout = vmlal_lane_s16(bout, r1.val[0], colorMatrix[2], 0);
                            bout = vmlal_lane_s16(bout, g1.val[0], colorMatrix[2], 1);
                            bout = vmlal_lane_s16(bout, b1.val[0], colorMatrix[2], 2);
                            
                            col16.val[0] = vqrshrun_n_s32(rout, 8);
                            col16.val[1] = vqrshrun_n_s32(gout, 8);
                            col16.val[2] = vqrshrun_n_s32(bout, 8);                     
                            col16.val[0] = vmin_u16(col16.val[0], bound);
                            col16.val[1] = vmin_u16(col16.val[1], bound);
                            col16.val[2] = vmin_u16(col16.val[2], bound);
                            vst3_u16(out16Ptr, col16);
                            out16Ptr += 12;
                            
                            r0 = r1;
                            g0 = g1;
                            b0 = b1;

                            i += 4;
                        }

                        // jump back
                        i -= VEC_WIDTH*4;

                        r0 = vzip_s16(vld1_s16(R_B(i)), vld1_s16(R_GB(i)));
                        g0 = vzip_s16(vld1_s16(G_B(i)), vld1_s16(G_GB(i)));
                        b0 = vzip_s16(vld1_s16(B_B(i)), vld1_s16(B_GB(i)));                     
                        i += 4;

                        for (int x = 1; x < VEC_WIDTH; x++) {
                            int16x4x2_t r1 = vzip_s16(vld1_s16(R_B(i)), vld1_s16(R_GB(i)));
                            int16x4x2_t g1 = vzip_s16(vld1_s16(G_B(i)), vld1_s16(G_GB(i)));
                            int16x4x2_t b1 = vzip_s16(vld1_s16(B_B(i)), vld1_s16(B_GB(i)));
                            
                            // do the color matrix
                            int32x4_t rout = vmovl_s16(vdup_lane_s16(colorMatrix[0], 3));                       
                            rout = vmlal_lane_s16(rout, r0.val[1], colorMatrix[0], 0);
                            rout = vmlal_lane_s16(rout, g0.val[1], colorMatrix[0], 1);
                            rout = vmlal_lane_s16(rout, b0.val[1], colorMatrix[0], 2);
                            
                            int32x4_t gout = vmovl_s16(vdup_lane_s16(colorMatrix[1], 3));                       
                            gout = vmlal_lane_s16(gout, r0.val[1], colorMatrix[1], 0);
                            gout = vmlal_lane_s16(gout, g0.val[1], colorMatrix[1], 1);
                            gout = vmlal_lane_s16(gout, b0.val[1], colorMatrix[1], 2);
                            
                            int32x4_t bout = vmovl_s16(vdup_lane_s16(colorMatrix[2], 3));
                            bout = vmlal_lane_s16(bout, r0.val[1], colorMatrix[2], 0);
                            bout = vmlal_lane_s16(bout, g0.val[1], colorMatrix[2], 1);
                            bout = vmlal_lane_s16(bout, b0.val[1], colorMatrix[2], 2);
                            
                            uint16x4x3_t col16;
                            col16.val[0] = vqrshrun_n_s32(rout, 8);
                            col16.val[1] = vqrshrun_n_s32(gout, 8);
                            col16.val[2] = vqrshrun_n_s32(bout, 8);                     
                            col16.val[0] = vmin_u16(col16.val[0], bound);
                            col16.val[1] = vmin_u16(col16.val[1], bound);
                            col16.val[2] = vmin_u16(col16.val[2], bound);
                            vst3_u16(out16Ptr, col16);
                            out16Ptr += 12;
                            
                            rout = vmovl_s16(vdup_lane_s16(colorMatrix[0], 3));                 
                            rout = vmlal_lane_s16(rout, r1.val[0], colorMatrix[0], 0);
                            rout = vmlal_lane_s16(rout, g1.val[0], colorMatrix[0], 1);
                            rout = vmlal_lane_s16(rout, b1.val[0], colorMatrix[0], 2);
                            
                            gout = vmovl_s16(vdup_lane_s16(colorMatrix[1], 3));                 
                            gout = vmlal_lane_s16(gout, r1.val[0], colorMatrix[1], 0);
                            gout = vmlal_lane_s16(gout, g1.val[0], colorMatrix[1], 1);
                            gout = vmlal_lane_s16(gout, b1.val[0], colorMatrix[1], 2);
                            
                            bout = vmovl_s16(vdup_lane_s16(colorMatrix[2], 3));
                            bout = vmlal_lane_s16(bout, r1.val[0], colorMatrix[2], 0);
                            bout = vmlal_lane_s16(bout, g1.val[0], colorMatrix[2], 1);
                            bout = vmlal_lane_s16(bout, b1.val[0], colorMatrix[2], 2);
                            
                            col16.val[0] = vqrshrun_n_s32(rout, 8);
                            col16.val[1] = vqrshrun_n_s32(gout, 8);
                            col16.val[2] = vqrshrun_n_s32(bout, 8);                     
                            col16.val[0] = vmin_u16(col16.val[0], bound);
                            col16.val[1] = vmin_u16(col16.val[1], bound);
                            col16.val[2] = vmin_u16(col16.val[2], bound);
                            vst3_u16(out16Ptr, col16);
                            out16Ptr += 12;
                            
                            r0 = r1;
                            g0 = g1;
                            b0 = b1;

                            i += 4;
                        }
                    }   
                    asm volatile("#End of stage 10) - color correction\n\t");
                }
                

                if (1) {

                    asm volatile("#Gamma Correction\n");                   
                    // Gamma correction (on the CPU, not the NEON)
                    const uint16_t * __restrict__ out16Ptr = out16;
                    
                    for (int y = 0; y < BLOCK_HEIGHT; y++) {                    
                        unsigned int * __restrict__ outPtr32 = (unsigned int *)(outBlockPtr + y * outWidth * 3);
                        for (int x = 0; x < (BLOCK_WIDTH*3)/4; x++) {
                            unsigned val = ((lut[out16Ptr[0]] << 0) |
                                            (lut[out16Ptr[1]] << 8) | 
                                            (lut[out16Ptr[2]] << 16) |
                                            (lut[out16Ptr[3]] << 24));
                            *outPtr32++ = val;
                            out16Ptr += 4;
                            // *outPtr++ = lut[*out16Ptr++];
                        }
                    }           
                    asm volatile("#end of Gamma Correction\n");                   
                    
                    /*
                    const uint16_t * __restrict__ out16Ptr = out16;                 
                    for (int y = 0; y < BLOCK_HEIGHT; y++) {                    
                        unsigned char * __restrict__ outPtr = (outBlockPtr + y * outWidth * 3);
                        for (int x = 0; x < (BLOCK_WIDTH*3); x++) {
                            *outPtr++ = lut[*out16Ptr++];
                        }
                    }
                    */
                    
                }
                

                blockPtr += BLOCK_WIDTH;
                outBlockPtr += BLOCK_WIDTH*3;
            }
        }       

        //std::cout << "Done demosaicking. time = " << ((Time::now() - startTime)/1000) << std::endl;
        return out;
    }

    Image makeThumbnailRAW_ARM(Frame src, float contrast, int blackLevel, float gamma) {
        // Assuming we want a slightly-cropped thumbnail into 640x480, Bayer pattern GRBG
        // This means averaging together a 4x4 block of Bayer pattern for one RGB24 pixel
        // Also want to convert to sRGB, which includes a color matrix multiply and a gamma transform
        // using a lookup table.

        // Implementation: 
        //   Uses ARM NEON SIMD vector instructions and inline assembly.
        //   Reads in a 16x4 block of pixels at a time, in 16-bit GRBG Bayer format, and outputs a 4x1 block of RGB24 pixels.
        // Important note: Due to some apparent bugs in GCC's inline assembly register mapping between C variables and NEON registers,
        //   namely that trying to reference an int16x4 variable creates a reference to a s register instead of a d register, all the
        //   int16x4 variables are forced into specific NEON registers, and then referred to using that register, not by name.  
        //   This bug seems to be in gcc 4.2.1, should be fixed by 4.4 based on some gcc bug reports.

        Image thumb(640, 480, RGB24);
        const unsigned int w = 2592, tw = 640;
        const unsigned int h = 1968, th = 480;
        const unsigned int scale = 4;
        const unsigned int cw = tw*scale;
        const unsigned int ch = th*scale;
        const unsigned int startX = (w-cw)/2;
        const unsigned int startY = (h-ch)/2;        
        const unsigned int bytesPerRow = src.image().bytesPerRow();

        // Make the response curve
        unsigned char lut[4096];
        makeLUT(src, contrast, blackLevel, gamma, lut);

        unsigned char *row = src.image()(startX, startY);

        Time startTime = Time::now();
        float colorMatrix_f[12];

        // Check if there's a custom color matrix
        if (src.shot().colorMatrix().size() == 12) {
            for (int i = 0; i < 12; i++) {
                colorMatrix_f[i] = src.shot().colorMatrix()[i];
            }
            printf("Making thumbnail with custom WB\n");
        } else {
            // Otherwise use the platform version
            src.platform().rawToRGBColorMatrix(src.shot().whiteBalance, colorMatrix_f);
            printf("Making thumbnail with platform WB\n");
        }

        register int16x4_t colorMatrix0 asm ("d0"); // ASM assignments are essential - they're implicitly trusted by the inline code.
        register int16x4_t colorMatrix1 asm ("d1");
        register int16x4_t colorMatrix2 asm ("d2");
        register int16x4_t wCoord asm ("d20"); // Workaround for annoyances with scalar addition.
        register int16x4_t maxValue asm ("d21"); // Maximum allowed signed 16-bit value
        register int16x4_t minValue asm ("d22"); // Minimum allowed signed 16-bit value

        asm volatile(//// Set up NEON constants in preselected registers
                    // Load matrix into colorMatrix0-2, set to be d0-d2
                    "vldm %[colorMatrix_f], {q2,q3,q4}  \n\t"
                    "vcvt.s32.f32 q2, q2, #8  \n\t" // Float->fixed-point conversion
                    "vcvt.s32.f32 q3, q3, #8  \n\t"
                    "vcvt.s32.f32 q4, q4, #8  \n\t"
                    "vmovn.i32 d0, q2  \n\t" // Narrowing to 16-bit
                    "vmovn.i32 d1, q3  \n\t"
                    "vmovn.i32 d2, q4  \n\t"
                    // Load homogenous coordinate, pixel value limits
                    "vmov.i16  d20, #0x4   \n\t"  // Homogenous coordinate. 
                    "vmov.i16  d21, #0x00FF  \n\t"  // Maximum pixel value: 1023
                    "vorr.i16  d21, #0x0300  \n\t"  // Maximum pixel value part 2
                    "vmov.i16  d22, #0x0     \n\t"  // Minimum pixel value: 0
                    : [colorMatrix0] "=w" (colorMatrix0),
                      [colorMatrix1] "=w" (colorMatrix1),
                      [colorMatrix2] "=w" (colorMatrix2),
                      [wCoord] "=w" (wCoord),
                      [maxValue] "=w" (maxValue),
                      [minValue] "=w" (minValue)
                    :  [colorMatrix_f] "r" (colorMatrix_f)
                    : "memory",
                      "d3", "d4", "d5", "d6", "d7", "d8", "d9");
                
        for (unsigned int ty = 0; ty <480; ty++, row+=4*bytesPerRow) {
            register unsigned short *px0 = (unsigned short *)row;
            register unsigned short *px1 = (unsigned short *)(row+1*bytesPerRow);
            register unsigned short *px2 = (unsigned short *)(row+2*bytesPerRow);
            register unsigned short *px3 = (unsigned short *)(row+3*bytesPerRow);

            register unsigned char *dst = thumb(0,ty);
            for (register unsigned int tx =0; tx < 640; tx+=scale) {
                // Assembly block for fast downsample/demosaic, color correction, and gamma curve lookup
                asm volatile(
                    // *px0: GRGR GRGR GRGR GRGR
                    // *px1: BGBG BGBG BGBG BGBG
                    // *px2: GRGR GRGR GRGR GRGR
                    // *px3: BGBG BGBG BGBG BGBG
                    //// Load a 16x4 block of 16-bit pixels from memory, de-interleave by 2 horizontally
                    "vld2.16 {d4-d7}, [%[px0]]!  \n\t"
                    "vld2.16 {d8-d11}, [%[px1]]! \n\t"
                    "vld2.16 {d12-d15}, [%[px2]]! \n\t"
                    "vld2.16 {d16-d19}, [%[px3]]! \n\t"
                    //  d4    d5    d6    d7
                    // GG|GG GG|GG RR|RR RR|RR
                    //  d8    d9    d10   d11
                    // BB|BB BB|BB GG|GG GG|GG
                    //  d12   d13   d14   d15
                    // GG|GG GG|GG RR|RR RR|RR
                    //  d16   d17   d18   d19
                    // BB|BB BB|BB GG|GG GG|GG

                    //// Accumulate neighbors together
                    "vpadd.u16 d4, d4, d5  \n\t"   // G1
                    "vpadd.u16 d5, d6, d7  \n\t"   // R1
                    "vpadd.u16 d6, d8, d9  \n\t"   // B1
                    "vpadd.u16 d7, d10, d11 \n\t"  // G2
                    "vpadd.u16 d8, d12, d13 \n\t"  // G3
                    "vpadd.u16 d9, d14, d15 \n\t"  // R2
                    "vpadd.u16 d10, d16, d17 \n\t" // B2
                    "vpadd.u16 d11, d18, d19 \n\t" // G4
                    //    d4       d5       d6       d7
                    // G|G|G|G  R|R|R|R  B|B|B|B  G|G|G|G
                    //    d8       d9       d10      d11
                    // G|G|G|G  R|R|R|R  B|B|B|B  G|G|G|G

                    //// Sum greens and partial normalize (using a vhadd)
                    "vadd.u16 d7, d8   \n\t"
                    "vadd.u16 d4, d11   \n\t"
                    "vhadd.u16 d4, d7  \n\t"
                    //// Sum red
                    "vadd.u16 d5, d9  \n\t"
                    //// Sum blue
                    "vadd.u16 d6, d10 \n\t"
                    //    d4       d5       d6  
                    // G|G|G|G  R|R|R|R  B|B|B|B
                    //
                    // Assuming sRGB affine matrix stored in fixed precision (lsb = 1/256)
                    // Trusting GCC to properly assign colorMatrix0-2 to d0-d2.  Direct reference seems to be broken on g++ 4.2.1 at least.
                    // r   colorMatrix0[0] [1] [2] [3]   r_in
                    // g = colorMatrix1[0] [1] [2] [3] * g_in
                    // b   colorMatrix2[0] [1] [2] [3]   b_in

                    //// Color convert multiply, extend to 32 bits

                    "vmull.s16 q4, d5, d0[0] \n\t"
                    "vmlal.s16 q4, d4, d0[1] \n\t"
                    "vmlal.s16 q4, d6, d0[2] \n\t"
                    "vmlal.s16 q4, d20, d0[3] \n\t" 

                    "vmull.s16 q5, d5, d1[0] \n\t"
                    "vmlal.s16 q5, d4, d1[1] \n\t"
                    "vmlal.s16 q5, d6, d1[2] \n\t"
                    "vmlal.s16 q5, d20, d1[3] \n\t"

                    "vmull.s16 q6, d5, d2[0] \n\t"
                    "vmlal.s16 q6, d4, d2[1] \n\t"
                    "vmlal.s16 q6, d6, d2[2] \n\t"
                    "vmlal.s16 q6, d20, d2[3] \n\t"

                    //  d08  d09  d10  d11  d12  d13
                    //  R|R  R|R  G|G  G|G  B|B  B|B

                    //// Normalize to 16-bit again, rounding. /4 for averaging, /256 for matrix fixed-point scaling
                    "vrshrn.s32 d3, q4, #10  \n\t"
                    "vrshrn.s32 d4, q5, #10  \n\t"
                    "vrshrn.s32 d5, q6, #10  \n\t"
                    //// Range limit pixel values
                    "vmin.s16 d3, d3, d21    \n\t"
                    "vmin.s16 d4, d4, d21    \n\t"
                    "vmin.s16 d5, d5, d21    \n\t"
                    "vmax.s16 d3, d3, d22    \n\t"
                    "vmax.s16 d4, d4, d22    \n\t"
                    "vmax.s16 d5, d5, d22    \n\t"

                    //    d3       d4       d2
                    // R|R|R|R  G|G|G|G  B|B|B|B
                    
                    //// Do gamma table without writeback. vmov are slow, but probably not as slow as an L1 write/read
                    "vmov r0,r1, d3                        \n\t"
                    //    r0       r1
                    // R16|R16  R16|R16
                    "uxth r2, r0                           \n\t" // Extract first red pixel into r2
                    "ldrb r4, [%[gammaTable], r2]          \n\t" // Table lookup, byte result

                    "uxth r2, r0, ROR #16                  \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r4, r4, r3, LSL #24              \n\t"

                    "uxth r2, r1                           \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "mov  r5, r3, LSL #16                  \n\t"

                    "uxth r2, r1, ROR #16                  \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "mov  r6, r3, LSL #8                   \n\t"

                    //   r4   r5   r6  
                    //  R__R __R_ _R__  -> increasing mem address (and increasing left shift)

                    "vmov r0,r1, d4                        \n\t"
                    //    r0       r1
                    // G16|G16  G16|G16
                    "uxth r2, r0                           \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r4, r4, r3, LSL #8               \n\t"

                    "uxth r2, r0, ROR #16                  \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r5, r5, r3                       \n\t"

                    "uxth r2, r1                           \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r5, r5, r3, LSL #24              \n\t"

                    "uxth r2, r1, ROR #16                  \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r6, r6, r3, LSL #16              \n\t"

                    //   r4   r5   r6  
                    //  RG_R G_RG _RG_  -> increasing mem address (and increasing left shift)

                    "vmov r0,r1, d5                        \n\t"
                    //    r0       r1
                    // B16|B16  B16|B16
                    "uxth r2, r0                           \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r4, r4, r3, LSL #16              \n\t"

                    "uxth r2, r0, ROR #16                  \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r5, r5, r3, LSL #8               \n\t"

                    "uxth r2, r1                           \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r6, r6, r3                       \n\t"

                    "uxth r2, r1, ROR #16                  \n\t"
                    "ldrb r3, [%[gammaTable], r2]          \n\t"
                    "orr  r6, r6, r3, LSL #24              \n\t"

                    //   r4   r5   r6  
                    //  RGBR GBRG BRGB 

                    "stm %[dst]!, {r4,r5,r6}                   \n\t" // multi-store!
                    : [px0] "+&r" (px0),
                      [px1] "+&r" (px1),
                      [px2] "+&r" (px2),
                      [px3] "+&r" (px3),
                      [dst] "+&r" (dst)
                    : [gammaTable] "r" (lut),
                      [colorMatrix0] "w" (colorMatrix0), // Implicitly referenced only (d0)
                      [colorMatrix1] "w" (colorMatrix1), // Implicitly referenced only (d1)
                      [colorMatrix2] "w" (colorMatrix2), // Implicitly referenced only (d2)
                      [wCoord] "w" (wCoord),             // Implicitly referenced only (d20)
                      [maxValue] "w" (maxValue),         // Implicitly referenced only (d21)
                      [minValue] "w" (minValue)          // Implicitly referenced only (d22)
                    : "memory", 
                      "r0", "r1", "r2", "r3", "r4", "r5", "r6",
                      "d3", "d4", "d5", "d6",
                      "d7", "d8", "d9", "d10", 
                      "d11", "d12", "d13", "d14",
                      "d15", "d16", "d17", "d18", "d19"
                    );

            }            
        }
        
        //std::cout << "Done creating fast thumbnail. time = " << ((Time::now()-startTime)/1000) << std::endl;

        return thumb;
    }
}


#endif
