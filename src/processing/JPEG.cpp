#include <stdio.h>

extern "C" {
#include <jpeglib.h>
}

#include <FCam/Event.h>
#include <FCam/processing/JPEG.h>
#include <FCam/processing/Demosaic.h>

#include "../Debug.h"

using namespace std;


namespace FCam {
    void saveJPEG(Image im, string filename, int quality) {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        dprintf(DBG_MINOR, "saveJPEG: Saving JPEG to %s, quality %d\n", filename.c_str(), quality);

        FILE *f = fopen(filename.c_str(), "wb");
        if (!f) {
            error(Event::FileSaveError, "saveJPEG: %s: Cannot open file for writing", filename.c_str());
            return;
        }
        
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, f);

        cinfo.image_width = im.width();
        cinfo.image_height = im.height();
        cinfo.input_components = 3;
        if (im.type() == RGB24) {
            cinfo.in_color_space = JCS_RGB;  
        } else if (im.type() == YUV24) {
            cinfo.in_color_space = JCS_YCbCr;
        }

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        if (im.type() == RGB24 || im.type() == YUV24) {
            while (cinfo.next_scanline < cinfo.image_height) {
                JSAMPLE *row = im(0, cinfo.next_scanline);
                jpeg_write_scanlines(&cinfo, &row, 1);
            }
        } else if (im.type() == UYVY) {
            std::vector<JSAMPLE> row(cinfo.image_width*3);
            while (cinfo.next_scanline < cinfo.image_height) {
                // convert the row to YUV
                JSAMPLE *rowPtr = &row[0];
                unsigned char *dataPtr = im(0, cinfo.next_scanline);
                for (size_t i = 0; i < cinfo.image_width/2; i++) {
                    rowPtr[0] = dataPtr[1];
                    rowPtr[1] = dataPtr[0];
                    rowPtr[2] = dataPtr[2];
                    rowPtr[3] = dataPtr[3];
                    rowPtr[4] = dataPtr[0];
                    rowPtr[5] = dataPtr[2];
                    rowPtr += 6;
                    dataPtr += 4;
                }
                rowPtr = &row[0];
                jpeg_write_scanlines(&cinfo, &rowPtr, 1);
            }
        } else if (im.type() == YUV420p) {

            std::vector<JSAMPLE> row(cinfo.image_width*3);
            while (cinfo.next_scanline < cinfo.image_height) {
                // convert the row to YUV
                JSAMPLE *rowPtr = &row[0];

                unsigned int  uvrow    = cinfo.next_scanline/4;
                unsigned int  uvcol    = cinfo.next_scanline%4 < 2 ? 0 : im.width()/2;
                unsigned char *dataYPtr = im(0, cinfo.next_scanline);
                unsigned char *dataUPtr = im(uvcol, im.height() + uvrow);
                unsigned char *dataVPtr = im(uvcol, im.height() + im.height()/4 + uvrow);

                for (size_t i = 0; i < cinfo.image_width/2; i++) {
                    rowPtr[0] = dataYPtr[0];
                    rowPtr[1] = dataUPtr[0];
                    rowPtr[2] = dataVPtr[0];
                    rowPtr[3] = dataYPtr[1];
                    rowPtr[4] = dataUPtr[0];
                    rowPtr[5] = dataVPtr[0];
                    rowPtr   += 6;
                    dataYPtr += 2;
                    dataUPtr++;
                    dataVPtr++;
                }
                rowPtr = &row[0];
                jpeg_write_scanlines(&cinfo, &rowPtr, 1);
            }
        }

        jpeg_finish_compress(&cinfo);
        fclose(f);
        jpeg_destroy_compress(&cinfo);

        dprintf(DBG_MINOR, "saveJPEG: Done saving JPEG to %s\n", filename.c_str());
    }

    void saveJPEG(Frame frame, string filename, int quality) {
        if (!frame.image().valid()) {
            error(Event::FileSaveError, frame, "saveJPEG: %s: No valid image in frame to save.", filename.c_str());
            return;
        }

        Image im = frame.image();
        
        switch (im.type()) {
        case RAW:
            im = demosaic(frame);
            if (!im.valid()) {
                error(Event::FileSaveError, frame, "saveJPEG: %s: Cannot demosaic RAW image to save as JPEG.", filename.c_str());
                return;
            }
            // fall through to rgb24
        case RGB24: case YUV24: case UYVY: case YUV420p:
            saveJPEG(im, filename, quality);
            break;
        default:
            error(Event::FileSaveError, frame, "saveJPEG: %s: Unsupported image format", filename.c_str());
            break;
        }
    }
};
