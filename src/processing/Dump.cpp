#include <stdio.h>

#include <FCam/Event.h>
#include <FCam/processing/Dump.h>

#include "../Debug.h"

namespace FCam {

    Image loadDump(std::string filename) {
        FILE *fp = fopen(filename.c_str(), "rb");

        if (!fp) {
            error(Event::FileLoadError, "loadDump: %s: Cannot open file for reading.", filename.c_str());
            return Image();
        }

        // Read the header
        int header[5];
        if (fread(header, sizeof(int), 5, fp) != 5) {
            error(Event::FileLoadError, "loadDump: %s: Unexpected EOF in header.", filename.c_str());
            fclose(fp);
            return Image();                                               
        }

        // sanity check the header
        if (header[0] != 1 || // frames
            header[1] < 0 ||  // width
            header[2] < 0) {
            error(Event::FileLoadError, "loadDump: %s: Malformed header.", filename.c_str());
            fclose(fp);
            return Image();
        }

        ImageFormat type;
        // check the number of channels is correct, given the type
        if (header[4] == 2 && header[3] == 2) {
            type = FCam::UYVY;
        } else if (header[4] == 4 && header[3] == 1) {
            type = FCam::RAW;
        } else if (header[4] == 2 && header[3] == 3) {
            type = FCam::RGB24;
        } else {
            error(Event::FileLoadError, "loadDump: %s: Malformed header.", filename.c_str());
            fclose(fp);            
            return Image();
        }

        // Allocate an image
        // todo: Allow loading into a preallocated image
        Image im(header[1], header[2], type);

        for (size_t y = 0; y < im.height(); y++) {
            size_t count = fread(im(0, y), bytesPerPixel(type), im.width(), fp);
            if (count != im.width()) {
                error(Event::FileLoadError, 
                      "loadDump: %s: Unexpected EOF in image data at line %d/%d.", 
                      filename.c_str(), y, im.height());
                fclose(fp);
                return Image();
            }
        }
        
        return im;
    }
    
    void saveDump(Frame f, std::string filename) {
        saveDump(f.image(), filename);
    }
    
    void saveDump(Image im, std::string filename) {
        dprintf(DBG_MINOR,"saveDump: Saving dump as %s.\n", filename.c_str());
        
        if (!im.valid()) {
            error(Event::FileSaveError, "saveDump: %s: Image to save not valid.", filename.c_str());
            return;
        }
        
        unsigned int frames = 1;
        unsigned int width = im.width();
        size_t widthBytes = width*im.bytesPerPixel();
        unsigned int height = im.height();
        unsigned int channels;
        unsigned int type;

        switch (im.type()) {
        case FCam::UYVY:
            type = 2;
            channels = 2;
            break;
        case FCam::RAW:
            type = 4;
            channels = 1; 
            break;
        case FCam::RGB24:
            type = 2;
            channels = 3;
            break;
        default:
            error(Event::FileSaveError, "saveDump: %s: Unknown image type.", filename.c_str());
            return;
        }
        dprintf(DBG_MINOR,"saveDump: %s: Header: %d %d %d %d %d. Bytes %d\n", filename.c_str(), 
                frames, width, height, channels, type, widthBytes*height+20);
        
        FILE *fp = fopen(filename.c_str(), "wb");
        if (!fp) {
            error(Event::FileSaveError, "saveDump: %s: Cannot open file for writing.", filename.c_str());
            return;
        }
        
        size_t count;
        int header[5] = {frames, width, height, channels, type};
        count = fwrite(header, sizeof(int), 5, fp);
        if (count != 5) {
            error(Event::FileSaveError, "saveDump: %s: Error writing header (out of space?)", filename.c_str());
            fclose(fp);
            return;
        }
    
        for (unsigned int y=0; y < height; y++) {
            count = fwrite(im(0,y), sizeof(char), widthBytes, fp);
            if (count != widthBytes) {
                error(Event::FileSaveError, "saveDump: %s: Error writing image data (out of space?)", filename.c_str());
                fclose(fp);
                return;
            }
        }

        dprintf(DBG_MINOR,"saveDump: %s: Done.\n", filename.c_str());
        fclose(fp);
    }

}

