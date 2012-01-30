#include <sstream>

#include "FCam/Frame.h"
#include "FCam/Action.h"
#include "FCam/Platform.h"
#include "Debug.h"

namespace FCam {
   
    // This function exists so that _Frame's vtable has an object file
    // to live in
    _Frame::_Frame(): exposure(0), frameTime(0), gain(0.0f), whiteBalance(5000) {}

    _Frame::~_Frame() {}
    Frame::~Frame() {}

    // Debugging dump function
    void _Frame::debug(const char *name) const {
        printf("\tDump of FCam::Frame %s at %llx:\n", name, (long long unsigned)this);
        printf("\t  Exposure start time: %s end time: %s\n", exposureStartTime.toString().c_str(), exposureEndTime.toString().c_str());
        printf("\t  Processing done time: %s\n", processingDoneTime.toString().c_str());
        printf("\t  Exposure: %d us, Frame time: %d us\n", exposure, frameTime);
        printf("\t  Gain: %f, White balance: %d K\n", gain, whiteBalance);
        printf("\t  Histogram details:\n");
        printf("\t\tValid: %s\n", histogram.valid() ? "yes" : "no");
        printf("\t\tBuckets: %d, Channels: %d\n", histogram.buckets(), histogram.channels());
        printf("\t\tRegion: (%d, %d) - (%d, %d)\n", histogram.region().x, histogram.region().y, 
               histogram.region().x+histogram.region().width, 
               histogram.region().y+histogram.region().height);
        printf("\t  Sharpness map details:\n");
        printf("\t\tValid: %s\n", sharpness.valid() ? "yes" : "no");
        printf("\t\tChannels: %d, Size: %d x %d\n", sharpness.channels(), sharpness.width(), sharpness.height());
        printf("\t  Camera RAW to sRGB(linear) conversion matrix at current white balance setting:\n");
        float matrix[16];
        platform().rawToRGBColorMatrix(whiteBalance, matrix);
        printf("\t\t[ [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[0], matrix[1], matrix[2], matrix[3]);
        printf("\t\t  [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[4], matrix[5], matrix[6], matrix[7]);
        printf("\t\t  [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[8], matrix[9], matrix[10], matrix[11]);
        printf("\t  Sensor bayer pattern: %s\n", (platform().bayerPattern() == RGGB ? "RGGB" :
                                                  platform().bayerPattern() == BGGR ? "BGGR" :
                                                  platform().bayerPattern() == GRBG ? "GRBG" :
                                                  platform().bayerPattern() == GBRG ? "GBRG" :
                                                  "Not Bayer"));
        printf("\t  Min raw value: %d, max raw value: %d\n", platform().minRawValue(), platform().maxRawValue() );
        printf("\t  Camera Model: %s, Manufacturer: %s\n", platform().model().c_str(), platform().manufacturer().c_str());
        printf("\t  Tag map contents:\n");
        for (TagMap::const_iterator it = tags.begin(); it != tags.end(); it++) {
            std::string val = (*it).second.toString();
            if (val.size() > 100) {
                std::stringstream s;
                s << val.substr(0,100) << "...(truncating " << val.size()-100 << " characters)";
                val = s.str();
            }
            printf("\t   Key: \"%s\" Value: %s\n", (*it).first.c_str(), val.c_str());
        }
        printf("\t  Requested shot contents:\n");
        printf("\t\tID: %d, wanted: %s\n", shot().id, shot().wanted ? "yes" : "no");
        printf("\t\tRequested exposure: %d, frame time: %d\n", shot().exposure, shot().frameTime);
        printf("\t\tRequested gain: %f, requested white balance: %d K\n", shot().gain, shot().whiteBalance);
        printf("\t\tRequested histogram configuration:\n");
        printf("\t\t\tEnabled: %s, buckets: %d\n", shot().histogram.enabled ? "yes" : "no", shot().histogram.buckets);
        printf("\t\t\tRegion: (%d, %d) - (%d, %d)\n", shot().histogram.region.x, shot().histogram.region.y, 
               shot().histogram.region.x+shot().histogram.region.width, 
               shot().histogram.region.y+shot().histogram.region.height);
        printf("\t\tRequested sharpness map configuration:\n");
        printf("\t\t\tEnabled: %s, size: %d x %d\n", shot().sharpness.enabled ? "yes" : "no", 
               shot().sharpness.size.width, shot().sharpness.size.height);
        printf("\t\tRequested actions:\n");
        for (std::set<Action *>::const_iterator it=shot().actions().begin(); it != shot().actions().end(); it++) {
            printf("\t\t\tAction object at %llx to fire at %d us into exposure, latency of %d us.\n", (long long unsigned)*it, (*it)->time, (*it)->latency);
        }
        printf("\t**  Dump of requested image object follows\n");
        shot().image.debug("Frame::Shot::image");

        printf("\t**  Dump of frame image data follows\n");
        image.debug("Frame::image");        
    }
}
