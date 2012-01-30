#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <cmath>
#include <errno.h>

#include <FCam/processing/DNG.h>
#include <FCam/processing/Color.h>
#include <FCam/processing/Demosaic.h>
#include <FCam/Event.h>
#include <FCam/Frame.h>
#include <FCam/Platform.h>

#include "TIFF.h"
#include "../Debug.h"

namespace FCam {

    _DNGFrame::_DNGFrame(): dngFile(NULL) {
        dngFile = new TiffFile;
    }

    _DNGFrame::~_DNGFrame() {
        delete dngFile;
    }

    void _DNGFrame::rawToRGBColorMatrix(int kelvin, float *matrix) const {
        if (dng.numIlluminants == 1) {
            for (int i=0;i < 12; i++) matrix[i] = dng.colorMatrix1[i];
            return;
        }
        // Linear interpolate using inverse CCT
        float alpha =
            (1./kelvin-1./dng.illuminant1)
            /(1./dng.illuminant2 - 1./dng.illuminant1);
        colorMatrixInterpolate(dng.colorMatrix1,
                               dng.colorMatrix2,
                               alpha, matrix);
    }

    void _DNGFrame::debug(const char *name) const {
        printf("\tDump of FCam::DNGFrame %s at %llx:\n", name, (long long unsigned)this);
        printf("\t  Source file name: %s\n", dngFile->filename().c_str());
        printf("\t  Number of calibration illuminants: %d\n", dng.numIlluminants);
        printf("\t  Illuminant 1: %d K, conversion matrix:\n", dng.illuminant1);
        const float *matrix = dng.colorMatrix1;
        printf("\t\t[ [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[0], matrix[1], matrix[2], matrix[3]);
        printf("\t\t  [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[4], matrix[5], matrix[6], matrix[7]);
        printf("\t\t  [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[8], matrix[9], matrix[10], matrix[11]);
        if (dng.numIlluminants == 2) {
            printf("\t Illuminant 2: %d K, conversion matrix:\n", dng.illuminant2);
            matrix = dng.colorMatrix2;
            printf("\t\t[ [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[0], matrix[1], matrix[2], matrix[3]);
            printf("\t\t  [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[4], matrix[5], matrix[6], matrix[7]);
            printf("\t\t  [ %5.3f %5.3f %5.3f %5.3f ]\n", matrix[8], matrix[9], matrix[10], matrix[11]);
        }
        printf("\t** Dump of cached DNGFrame thumbnail image data follows\n");
        thumbnail.debug("DNGFrame::thumbnail");
        printf("\t** Dump of base Frame fields follows\n");
        FCam::_Frame::debug("base frame");
    }

    DNGFrame::DNGFrame(_DNGFrame *f): FCam::Frame(f) {}
    
    Image DNGFrame::thumbnail() { 
        return get()->thumbnail;
    }
    
    const char tiffEPVersion[4] = {1,0,0,0};
    const char understoodDNGVersion[4] = {1,3,0,0};
    const char oldestSupportedDNGVersion[4] = {1,2,0,0};
    const char privateDataPreamble[] = "stanford.fcam.privatedata";
    const int privateDataVersion = 2;
    const int backwardPrivateDataVersion = 2;

    // Notes on privateDataVersion:
    // version 1:
    //   Initial release
    // version 2:
    //   Added backwards-compatible version field. Readers need to check this to see if they can parse the data.
    //   All frame data now stored as a key/value set (a serialized TagMap, in other words)
    //   Frame members have known tag names that are searched for and removed from the TagMap. Anything unknown is left,
    //   which includes application-added tags and possible frame members from newer implementations.

    void saveDNGPrivateData_v1(const Frame &frame, std::stringstream &privateData,
                               const std::vector<float> &rawToRGB3000, const std::vector<float> &rawToRGB6500);
    void saveDNGPrivateData_v1(const Frame &frame, std::stringstream &privateData,
                               const std::vector<float> &rawToRGB3000, const std::vector<float> &rawToRGB6500) {
        privateData << TagValue(frame.exposureStartTime()).toBlob();
        privateData << TagValue(frame.exposureEndTime()).toBlob();
        privateData << TagValue(frame.processingDoneTime()).toBlob();
        privateData << TagValue(frame.exposure()).toBlob();
        privateData << TagValue(frame.frameTime()).toBlob();
        privateData << TagValue(frame.gain()).toBlob();
        privateData << TagValue(frame.whiteBalance()).toBlob();
        
        privateData << TagValue(frame.shot().exposure).toBlob();
        privateData << TagValue(frame.shot().frameTime).toBlob();
        privateData << TagValue(frame.shot().gain).toBlob();
        privateData << TagValue(frame.shot().whiteBalance).toBlob();
        
        privateData << TagValue(frame.platform().minRawValue()).toBlob();
        privateData << TagValue(frame.platform().maxRawValue()).toBlob();
        
        privateData << TagValue(3000).toBlob();
        privateData << TagValue(rawToRGB3000).toBlob();
        privateData << TagValue(6500).toBlob();
        privateData << TagValue(rawToRGB6500).toBlob();
        
        // (not writing histogram/sharpness map)
        
        // Write tags
        for (TagMap::const_iterator it = frame.tags().begin(); it != frame.tags().end(); it++) {
            privateData << TagValue(it->first) << TagValue(it->second);
        }
        
    }

    void saveDNGPrivateData_v2(const Frame &frame, std::stringstream &privateData, 
                               const std::vector<float> &rawToRGB3000, const std::vector<float> &rawToRGB6500);
    void saveDNGPrivateData_v2(const Frame &frame, std::stringstream &privateData, 
                               const std::vector<float> &rawToRGB3000, const std::vector<float> &rawToRGB6500) {

        // First write backward-compatibility field
        privateData << TagValue(backwardPrivateDataVersion).toBlob();
        
        // Now write everything using a TagMap
        TagMap frameFields;
        frameFields["frame.exposureStartTime"] = frame.exposureStartTime();
        frameFields["frame.exposureEndTime"] = frame.exposureEndTime();

        frameFields["frame.processingDoneTime"] = frame.processingDoneTime();
        frameFields["frame.exposure"] = frame.exposure();
        frameFields["frame.frameTime"] = frame.frameTime();
        frameFields["frame.gain"] = frame.gain();
        frameFields["frame.whiteBalance"] = frame.whiteBalance();
        
        frameFields["frame.shot.exposure"] = frame.shot().exposure;
        frameFields["frame.shot.frameTime"] = frame.shot().frameTime;
        frameFields["frame.shot.gain"] = frame.shot().gain;
        frameFields["frame.shot.whiteBalance"] = frame.shot().whiteBalance;
        frameFields["frame.shot.colorMatrix"] = frame.shot().colorMatrix();

        frameFields["frame.platform.minRawValue"] = frame.platform().minRawValue();
        frameFields["frame.platform.maxRawValue"] = frame.platform().maxRawValue();

        frameFields["frame.illuminant1"] = 3000;        
        frameFields["frame.colorMatrix1"] = rawToRGB3000;
        frameFields["frame.illuminant2"] = 6500;        
        frameFields["frame.colorMatrix2"] = rawToRGB6500;
                
        // Write frame fields
        for (TagMap::const_iterator it = frameFields.begin(); it != frameFields.end(); it++) {
            privateData << TagValue(it->first).toBlob() << it->second.toBlob();
        }

        // Write tags
        for (TagMap::const_iterator it = frame.tags().begin(); it != frame.tags().end(); it++) {
            privateData << TagValue(it->first).toBlob() << it->second.toBlob();
        }
    }

    void saveDNG(Frame frame, const std::string &filename) {
        dprintf(DBG_MINOR, "saveDNG: Starting to write %s\n", filename.c_str());

        // Initial error checking

        if (!frame.valid()) {
            error(Event::FileSaveError, frame,
                  "saveDNG: Cannot save invalid frame as %s.", filename.c_str());
            return;
        }
        if (!frame.image().valid()) {
            error(Event::FileSaveError, frame,
                  "saveDNG: Cannot save frame with no valid image as %s.", filename.c_str());
            return;
        }
        if (frame.image().type() != RAW) {
            error(Event::FileSaveError, frame,
                  "saveDNG: Cannot save a non-RAW frame as a DNG %s", filename.c_str());
            return;
        }
        if (frame.platform().bayerPattern() == NotBayer) {
            error(Event::FileSaveError, frame,
                  "saveDNG: Cannot save non-Bayer pattern RAW data as %s", filename.c_str());
            return;
        }

        // Figure out the color matrices for this sensor
        std::vector<float> rawToRGB3000(12);
        std::vector<float> rawToRGB6500(12);
        frame.platform().rawToRGBColorMatrix(3000, &rawToRGB3000.front());
        frame.platform().rawToRGBColorMatrix(6500, &rawToRGB6500.front());

        // First map from raw to XYZ
        std::vector<double> rawToXYZ3000(9);
        std::vector<double> rawToXYZ6500(9);
        std::vector<double> rawToRGB3000_linear(9);
        std::vector<double> rawToRGB6500_linear(9);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                rawToRGB3000_linear[i*3+j] = rawToRGB3000[i*4+j];
                rawToRGB6500_linear[i*3+j] = rawToRGB6500[i*4+j];
                for (int k = 0; k < 3; k++) {
                    rawToXYZ3000[i*3+j] += FCam::RGBtoXYZ[i*3+k]*rawToRGB3000[k*4+j];
                    rawToXYZ6500[i*3+j] += FCam::RGBtoXYZ[i*3+k]*rawToRGB6500[k*4+j];
                }
            }
        }

        std::vector<double> xyzToRaw3000(9);
        std::vector<double> xyzToRaw6500(9);
        // Then invert
        invert3x3(&rawToXYZ3000.front(), &xyzToRaw3000.front());
        invert3x3(&rawToXYZ6500.front(), &xyzToRaw6500.front());

        // Need to back-calculate black levels as raw sensor values as well
        // Black levels may vary as a function of illuminant (glare?), so calculate black level as a function of frame white balance
        std::vector<double> rgbToRAW3000(9);
        std::vector<double> rgbToRAW6500(9);
        invert3x3(&rawToRGB3000_linear.front(), &rgbToRAW3000.front() );
        invert3x3(&rawToRGB6500_linear.front(), &rgbToRAW6500.front() );
        std::vector<double> blackLevel3000(3), blackLevel6500(3);
        for (int i=0; i < 3; i++) {
            for (int j=0; j < 3; j++) {
                blackLevel3000[i] += rgbToRAW3000[i*3+j] * rawToRGB3000[3+j*4];
                blackLevel6500[i] += rgbToRAW6500[i*3+j] * rawToRGB6500[3+j*4];
            }
        }
        // linear interpolation with inverse correlated color temperature
        float wbInv = 1.0/frame.whiteBalance();
        float kInv3000 = 1.0/3000.f;
        float kInv6500 = 1.0/6500.f;

        float alpha = (wbInv - kInv6500)/(kInv3000-kInv6500);
        std::vector<int> blackLevel(3);
        for (int i=0; i < 3; i++) {
            blackLevel[i] = static_cast<int>(-std::floor(alpha*blackLevel6500[i]+(1-alpha)*blackLevel3000[i]+0.5));
        }
        dprintf(5, "saveDNG: Black level 6500k: %f %f %f, 3000k: %f %f %f, WB: %d k, alpha: %f, final black level: %d %d %d\n",
                blackLevel6500[0],blackLevel6500[1],blackLevel6500[2],
                blackLevel3000[0],blackLevel3000[1],blackLevel3000[2],
                frame.whiteBalance(),
                alpha,
                blackLevel[0], blackLevel[1], blackLevel[2]);
  
        // Start constructing DNG fields

        TiffFile dng;

        TiffIfd *ifd0 = dng.addIfd();

        // Add IFD0 entries

        std::string dngVersion(understoodDNGVersion,4);
        ifd0->add(DNG_TAG_DNGVersion, dngVersion);
        
        std::string dngBackVersion(oldestSupportedDNGVersion,4);
        ifd0->add(DNG_TAG_DNGBackwardVersion, dngBackVersion);

        ifd0->add(TIFF_TAG_Make, frame.platform().manufacturer());
        ifd0->add(TIFF_TAG_Model, frame.platform().model());
        ifd0->add(DNG_TAG_UniqueCameraModel, frame.platform().model());

        if (frame.tags().find("flash.brightness") != frame.tags().end()) {
            // \todo Find actual spec on this, implement better
            // bit
            //       0  : 0 = flash didn't fire, 1 = flash fired
            //     21   : 00 = no strobe return detect, 01 = reserved, 10 = strobe return not detected, 11 = strobe return detected
            //   43     : 00 = unknown, 01 = compulsory flash, 10 = compulsory flash supress, 11 = auto mode
            //  5       : 0 = flash function present, 1 = no flash function
            // 6        : 0 = no red-eye reduction, 1 = red-eye reduction supported
            ifd0->add(TIFFEP_TAG_Flash, 1);
        }

        std::string tiffEPStandardID(tiffEPVersion, 4);
        ifd0->add(TIFFEP_TAG_TIFFEPStandardID, tiffEPStandardID);

        time_t tim = frame.exposureStartTime().s();
        struct tm *local = localtime(&tim);
        char buf[20];
        snprintf(buf, 20, "%04d:%02d:%02d %02d:%02d:%02d",
                 local->tm_year + 1900,
                 local->tm_mon+1,
                 local->tm_mday,
                 local->tm_hour,
                 local->tm_min,
                 local->tm_sec);
        ifd0->add(TIFF_TAG_DateTime, std::string(buf));

        ifd0->add(DNG_TAG_CalibrationIlluminant1, DNG_TAG_CalibrationIlluminant_StdA);
        ifd0->add(DNG_TAG_ColorMatrix1, xyzToRaw3000);

        ifd0->add(DNG_TAG_CalibrationIlluminant2, DNG_TAG_CalibrationIlluminant_D65);
        ifd0->add(DNG_TAG_ColorMatrix2, xyzToRaw6500);

        ifd0->add(TIFF_TAG_Orientation, TIFF_Orientation_TopLeft);

        std::vector<double> whiteXY(2);
        float x,y;
        kelvinToXY(frame.whiteBalance(), &x, &y);
        whiteXY[0] = x; whiteXY[1] = y;
        ifd0->add(DNG_TAG_AsShotWhiteXY, whiteXY);

        std::vector<double> lensInfo(4);
        if (frame.tags().find("lens.minZoom") != frame.tags().end()) {
            lensInfo[0] = frame["lens.minZoom"].asFloat();
            lensInfo[1] = frame["lens.maxZoom"].asFloat();
            lensInfo[2] = frame["lens.wideApertureMin"].asFloat();
            lensInfo[3] = frame["lens.wideApertureMax"].asFloat();
            ifd0->add(DNG_TAG_LensInfo, lensInfo);
        }
        
        // Add some EXIF tags
        TiffIfd *exifIfd = ifd0->addExifIfd();

        exifIfd->add(EXIF_TAG_ExposureTime, double(frame.exposure())/1e6);
        if (frame.tags().find("lens.aperture") != frame.tags().end()) {
            double fNumber = frame["lens.aperture"].asFloat();
            ifd0->add(EXIF_TAG_FNumber, fNumber);
        }

        int usecs = frame.exposureStartTime().us();
        snprintf(buf, 20, "%06d", usecs);
        exifIfd->add(EXIF_TAG_SubsecTime, std::string(buf));

        exifIfd->add(EXIF_TAG_SensitivityType, EXIF_TAG_SensitivityType_ISO);
        exifIfd->add(EXIF_TAG_ISOSpeedRatings, (int)(frame.gain()*100) );

        if (frame.tags().find("lens.zoom") != frame.tags().end()) {
            double focalLength = frame["lens.zoom"].asFloat();
            exifIfd->add(EXIF_TAG_FocalLength, focalLength);
        }

        // Create our very own DNG private data!
        {
            std::stringstream privateData;
            // must start with manufacturer and identification string, terminated by null character. No whitespace!
            privateData << privateDataPreamble << std::ends;
            // never remove this
            privateData << TagValue(privateDataVersion).toBlob();
            switch (privateDataVersion) {
            case 1:
                saveDNGPrivateData_v1(frame, privateData, rawToRGB3000, rawToRGB6500);
                break;
            case 2:
                saveDNGPrivateData_v2(frame, privateData, rawToRGB3000, rawToRGB6500);
                break;
            }
            ifd0->add(DNG_TAG_DNGPrivateData, privateData.str());
        }
        // Add thumbnail into thumbnail IFD

        dprintf(4, "saveDNG: Adding thumbnail\n");
        TiffIfd *thumbIfd = ifd0;
        Image thumbnail = makeThumbnail(frame);

        thumbIfd->add(TIFF_TAG_NewSubFileType, (int)TIFF_NewSubfileType_MainPreview);
        thumbIfd->setImage(thumbnail);

        // Add RAW Ifd and entries
        dprintf(4, "saveDNG: Adding RAW metadata\n");

        TiffIfd *rawIfd = ifd0->addSubIfd();

        rawIfd->add(TIFF_TAG_NewSubFileType, (int)TIFF_NewSubfileType_FullRAW);

        std::vector<int> CFARepeatPatternDim(2,2);
        rawIfd->add(TIFFEP_TAG_CFARepeatPatternDim, CFARepeatPatternDim);

        std::string CFAPattern;
        std::vector<int> blackLevelPattern;
        switch(frame.platform().bayerPattern()) {
        case RGGB:
            CFAPattern = std::string(TIFFEP_CFAPattern_RGGB,4);
            blackLevelPattern.push_back(blackLevel[0]);
            blackLevelPattern.push_back(blackLevel[1]);
            blackLevelPattern.push_back(blackLevel[1]);
            blackLevelPattern.push_back(blackLevel[2]);
            break;
        case BGGR:
            CFAPattern = std::string(TIFFEP_CFAPattern_BGGR,4);
            blackLevelPattern.push_back(blackLevel[2]);
            blackLevelPattern.push_back(blackLevel[1]);
            blackLevelPattern.push_back(blackLevel[1]);
            blackLevelPattern.push_back(blackLevel[0]);
            break;
        case GRBG:
            CFAPattern = std::string(TIFFEP_CFAPattern_GRBG,4);
            blackLevelPattern.push_back(blackLevel[1]);
            blackLevelPattern.push_back(blackLevel[0]);
            blackLevelPattern.push_back(blackLevel[2]);
            blackLevelPattern.push_back(blackLevel[1]);
            break;
        case GBRG:
            CFAPattern = std::string(TIFFEP_CFAPattern_GBRG,4);
            blackLevelPattern.push_back(blackLevel[1]);
            blackLevelPattern.push_back(blackLevel[2]);
            blackLevelPattern.push_back(blackLevel[0]);
            blackLevelPattern.push_back(blackLevel[1]);
        default:
            error(Event::FileSaveError, "saveDNG: %s: Can't handle non-bayer RAW images", filename.c_str());
            return;
            break;
        }
        rawIfd->add(TIFFEP_TAG_CFAPattern, CFAPattern);
        rawIfd->add(DNG_TAG_BlackLevel, blackLevelPattern);

        std::vector<int> blackLevelRepeatDim(2,2);
        rawIfd->add(DNG_TAG_BlackLevelRepeatDim, blackLevelRepeatDim);

        rawIfd->add(DNG_TAG_WhiteLevel, frame.platform().maxRawValue());

        // Leaving a 4-pixel border around RAW image to allow for loss of data during interpolation
        std::vector<int> cropOrigin(2);
        cropOrigin[0] = 4;
        cropOrigin[1] = 4;
        std::vector<int> cropSize(2);
        cropSize[0] = frame.image().width()-8;
        cropSize[1] = frame.image().height()-8;

        rawIfd->add(DNG_TAG_DefaultCropOrigin, cropOrigin);
        rawIfd->add(DNG_TAG_DefaultCropSize, cropSize);

        dprintf(4, "saveDNG: Adding RAW image\n");
        rawIfd->setImage(frame.image());

        dprintf(4, "saveDNG: Beginning write to disk\n");
        // Constructed all DNG fields, write it to disk
        dng.writeTo(filename);

        dprintf(DBG_MINOR, "saveDNG: Done writing %s\n", filename.c_str());
    }

    void loadDNGPrivateData_v1(_DNGFrame *_f, std::stringstream &privateData);
    void loadDNGPrivateData_v1(_DNGFrame *_f, std::stringstream &privateData) {
        // Deserialize our fields now
        // Order below must match DNG save order
        TagValue val;
        privateData >> val;
        _f->exposureStartTime = val;
        privateData >> val;
        _f->exposureEndTime = val;
        privateData >> val;
        _f->processingDoneTime = val;
        privateData >> val;
        _f->exposure = val;
        privateData >> val;
        _f->frameTime = val;
        privateData >> val;
        _f->gain = val;
        privateData >> val;
        _f->whiteBalance = val;
        
        privateData >> val;
        _f->_shot.exposure = val;
        privateData >> val;
        _f->_shot.frameTime = val;
        privateData >> val;
        _f->_shot.gain = val;
        privateData >> val;
        _f->_shot.whiteBalance = val;
        
        privateData >> val;
        _f->dng.minRawValue = val.asInt();
        privateData >> val;
        _f->dng.maxRawValue = val.asInt();
        
        _f->dng.numIlluminants = 2;
        
        privateData >> val;
        _f->dng.illuminant1 = val;
        privateData >> val;
        for (int i=0; i < 12 ; i++)
            _f->dng.colorMatrix1[i] = val.asFloatVector()[i];
        
        privateData >> val;
        _f->dng.illuminant2 = val;
        privateData >> val;
        for (int i=0; i < 12 ; i++)
            _f->dng.colorMatrix2[i] = val.asFloatVector()[i];
        
        // And the tags
        TagValue key;
        while ((privateData >> key).good()) {
            privateData >> val;
            _f->tags[key] = val;
        }
    }

    void loadDNGPrivateData_v2(_DNGFrame *_f, std::stringstream &privateData);
    void loadDNGPrivateData_v2(_DNGFrame *_f, std::stringstream &privateData) {

        // Read everything into the frame tags
        TagValue key, val;
        while ((privateData >> key).good()) {
            privateData >> val;
            _f->tags[key] = val;
        }

        // And then look for special frame fields and parse them out

        TagMap::iterator it;

#define grabField(fieldName, fieldDest, asType)      \
        do {                                         \
            it = _f->tags.find(fieldName);           \
            if ( it != _f->tags.end()) {             \
                fieldDest = it->second.asType;       \
                _f->tags.erase(it);                  \
            }                                        \
        } while(0)

        grabField("frame.exposureStartTime", _f->exposureStartTime, asTime());
        grabField("frame.exposureEndTime", _f->exposureEndTime, asTime());

        grabField("frame.processingDoneTime", _f->processingDoneTime, asTime());
        grabField("frame.exposure", _f->exposure, asInt());
        grabField("frame.frameTime", _f->frameTime, asInt());
        grabField("frame.gain", _f->gain, asFloat());

        grabField("frame.whiteBalance", _f->whiteBalance, asInt());

        grabField("frame.shot.exposure", _f->_shot.exposure, asInt());
        grabField("frame.shot.frameTime", _f->_shot.frameTime, asInt());
        grabField("frame.shot.gain", _f->_shot.gain, asFloat());

        grabField("frame.shot.whiteBalance", _f->_shot.whiteBalance, asInt());
        
        std::vector<float> cMatrix;
        grabField("frame.shot.colorMatrix", cMatrix, asFloatVector());
        _f->_shot.setColorMatrix(cMatrix);

        grabField("frame.platform.minRawValue", _f->dng.minRawValue, asInt());
        grabField("frame.platform.maxRawValue", _f->dng.maxRawValue, asInt());

        _f->dng.numIlluminants = 2;
        grabField("frame.illuminant1", _f->dng.illuminant1, asInt());
        grabField("frame.illuminant2", _f->dng.illuminant2, asInt());

        it = _f->tags.find("frame.colorMatrix1");
        if (it != _f->tags.end()) {
            for (int i=0; i < 12; i++) {
                _f->dng.colorMatrix1[i] = it->second.asFloatVector()[i];
            }
            _f->tags.erase(it);
        }

        it = _f->tags.find("frame.colorMatrix2");
        if (it != _f->tags.end()) {
            for (int i=0; i < 12; i++) {
                _f->dng.colorMatrix2[i] = it->second.asFloatVector()[i];
            }
            _f->tags.erase(it);
        }

        // All leftover tags stay in the frame tags
    }


    DNGFrame loadDNG(const std::string &filename) {
        // Construct DNG Frame
        _DNGFrame *_f = new _DNGFrame;
        DNGFrame f(_f);
        // We'll mostly manipulate the internal dngFile for now
        TiffFile &dng = *(_f->dngFile);

        // Start reading DNG file data
        dng.readFrom(filename);

        if (!dng.valid) {
            // An error during initial TIFF parsing is recorded in
            // dng.lastEvent, forward it to the event system
            Event err = dng.lastEvent;
            DNGFrame nullF;
            err.creator = nullF;
            postEvent(err);
            return nullF;
        }

        // Convenience error macro
#define fatalError(...) do {                                            \
            DNGFrame nullF;                                             \
            error(Event::FileLoadError, nullF, __VA_ARGS__);            \
            return nullF; } while(0)

        //
        // Verify DNG version

        const TiffIfdEntry *versionEntry, *backVersionEntry;

        versionEntry = dng.ifds(0)->find(DNG_TAG_DNGVersion);
        if (!versionEntry) fatalError("loadDNG: File %s is not a valid DNG file (no DNG version found)", filename.c_str());

        std::string ver(versionEntry->value());
        dprintf(4, "loadDNG: DNG file version is %d.%d.%d.%d.\n", ver[0],ver[1],ver[2],ver[3]);

        backVersionEntry = dng.ifds(0)->find(DNG_TAG_DNGBackwardVersion);
        if (!backVersionEntry) {
            // Default: DNGVersion with last two values empty
            ver = (std::string)backVersionEntry->value();
            ver[2] = 0;
            ver[3] = 0;
        } else {
            //ver = (std::string)dng.parseEntry(*backVersionEntry);
            ver = (std::string)backVersionEntry->value();
        }
        dprintf(4, "loadDNG: DNG file backward version is %d.%d.%d.%d.\n", ver[0], ver[1], ver[2], ver[3]);

        if ( (understoodDNGVersion[0] < ver[0]) ||
             ( (understoodDNGVersion[0] == ver[0]) && (understoodDNGVersion[1] < ver[1])) ||
             ( (understoodDNGVersion[1] == ver[1]) && (understoodDNGVersion[2] < ver[2])) ||
             ( (understoodDNGVersion[2] == ver[2]) && (understoodDNGVersion[3] < ver[3]) ) ) {
            fatalError("loadDNG: File %s is too new, requires understanding at least DNG version %d.%d.%d.%d\n", filename.c_str(), ver[0],ver[1],ver[2],ver[3]);
        }

        //
        // Let's find the RAW IFD

        const TiffIfdEntry *entry;
        entry = dng.ifds(0)->find(TIFF_TAG_NewSubFileType);
        int ifdType = TIFF_NewSubfileType_DEFAULT;
        if (entry) {
            ifdType = entry->value();
        }

        TiffIfd *rawIfd = NULL;
        if (ifdType == (int)TIFF_NewSubfileType_FullRAW) {
            // Main IFD has RAW data
            dprintf(4, "loadDNG: RAW data found in IFD0\n");
            rawIfd = dng.ifds(0);
        } else {
            // Search subIFDs for RAW data
            for (size_t i=0; i < dng.ifds(0)->subIfds().size(); i++) {
                ifdType = TIFF_NewSubfileType_DEFAULT;
                entry = dng.ifds(0)->subIfds(i)->find(TIFF_TAG_NewSubFileType);
                if (entry) {
                    ifdType = entry->value();
                }
                if (ifdType == (int)TIFF_NewSubfileType_FullRAW) {
                    dprintf(4, "loadDNG: RAW data found in subIFD %d\n", i);
                    rawIfd = dng.ifds(0)->subIfds(i);
                    break;
                }
            }
            if (!rawIfd) fatalError("loadDNG: %s: Can't find RAW data!", filename.c_str());
        }

        //
        // Read in RAW image data 
        _f->image = rawIfd->getImage();
        
        //
        // Ok, now to parse the RAW metadata

        // Read in Bayer mosaic pattern
        entry = rawIfd->find(TIFFEP_TAG_CFARepeatPatternDim);
        if (!entry) fatalError("loadDNG: %s: No CFA pattern dimensions tag found.");
        BayerPattern bayer;
        {
            std::vector<int> &cfaRepeatPatternDim = entry->value();
            if (cfaRepeatPatternDim[0] != 2 ||
                cfaRepeatPatternDim[1] != 2) fatalError("loadDNG: %s: CFA pattern dimensions not 2x2 (%d x %d). Unsupported.\n", filename.c_str());

            entry = rawIfd->find(TIFFEP_TAG_CFAPattern);
            if (!entry) fatalError("loadDNG: %s: No CFA pattern tag found.");
            std::string cfaPattern = entry->value();
            if (cfaPattern.size() != 4) fatalError("loadDNG: %s: CFA pattern entry incorrect\n", filename.c_str());
            if (cfaPattern[0] == TIFFEP_CFAPattern_RGGB[0] &&
                cfaPattern[1] == TIFFEP_CFAPattern_RGGB[1] &&
                cfaPattern[2] == TIFFEP_CFAPattern_RGGB[2] &&
                cfaPattern[3] == TIFFEP_CFAPattern_RGGB[3]) {
                bayer = RGGB;
            } else if (cfaPattern[0] == TIFFEP_CFAPattern_BGGR[0] &&
                       cfaPattern[1] == TIFFEP_CFAPattern_BGGR[1] &&
                       cfaPattern[2] == TIFFEP_CFAPattern_BGGR[2] &&
                       cfaPattern[3] == TIFFEP_CFAPattern_BGGR[3]) {
                bayer = BGGR;
            } else if (cfaPattern[0] == TIFFEP_CFAPattern_GRBG[0] &&
                       cfaPattern[1] == TIFFEP_CFAPattern_GRBG[1] &&
                       cfaPattern[2] == TIFFEP_CFAPattern_GRBG[2] &&
                       cfaPattern[3] == TIFFEP_CFAPattern_GRBG[3]) {
                bayer = GRBG;
            } else if (cfaPattern[0] == TIFFEP_CFAPattern_GBRG[0] &&
                       cfaPattern[1] == TIFFEP_CFAPattern_GBRG[1] &&
                       cfaPattern[2] == TIFFEP_CFAPattern_GBRG[2] &&
                       cfaPattern[3] == TIFFEP_CFAPattern_GBRG[3]) {
                bayer = GBRG;
            } else {
                fatalError("loadDNG: %s: Unsupported CFA pattern: %d %d %d %d\n", filename.c_str(), cfaPattern[0], cfaPattern[1], cfaPattern[2], cfaPattern[3]);
            }
            dprintf(4,"loadDNG: %s: CFA pattern is type %d\n", filename.c_str(), bayer);
        }

        // Read in black level
        entry = rawIfd->find(DNG_TAG_BlackLevelRepeatDim);
        uint16_t blackLevelRepeatRows = 1;
        uint16_t blackLevelRepeatCols = 1;
        if (entry) {
            std::vector<int> &blackLevelDims = entry->value();
            blackLevelRepeatRows = blackLevelDims[0];
            blackLevelRepeatCols = blackLevelDims[1];
        }
        if (! ((blackLevelRepeatRows == 1 && blackLevelRepeatCols == 1)
               || (blackLevelRepeatRows == 2 && blackLevelRepeatCols == 2)) ) {
            fatalError("loadDNG: %s: Black level pattern size is unsupported: %d x %d (only support 1x1 or 2x2)",
                       filename.c_str(), blackLevelRepeatRows, blackLevelRepeatCols);
        }

        entry = rawIfd->find(DNG_TAG_BlackLevel);
        uint16_t blackLevel[3] = {0,0,0};
        if (entry) {
            if (entry->value().type == TagValue::Int) {
                if (blackLevelRepeatRows != 1) {
                    fatalError("loadDNG: %s: Mismatched entry count between BlackLevelRepeatDim and BlackLevel",
                               filename.c_str());
                }
                blackLevel[0] = (int)entry->value();
                blackLevel[1] = (int)entry->value();
                blackLevel[2] = (int)entry->value();
            } else if (entry->value().type == TagValue::IntVector) {
                std::vector<int> &blackLevelTag = entry->value();
                if (blackLevelRepeatRows != 2 ||
                    blackLevelTag.size() != 4) {
                    fatalError("loadDNG: %s: Mismatched entry count between BlackLevelRepeatDim and BlackLevel",
                               filename.c_str());
                }
                switch(bayer) {
                    case RGGB:
                        blackLevel[0] = (int)blackLevelTag[0];
                        blackLevel[1] = (int)(blackLevelTag[1] + blackLevelTag[2])/2;
                        blackLevel[2] = (int)blackLevelTag[3];
                        break;
                    case BGGR:
                        blackLevel[0] = blackLevelTag[3];
                        blackLevel[1] = (blackLevelTag[1] + blackLevelTag[2])/2;
                        blackLevel[2] = blackLevelTag[0];
                        break;
                    case GRBG:
                        blackLevel[0] = blackLevelTag[1];
                        blackLevel[1] = (blackLevelTag[0] + blackLevelTag[3])/2;
                        blackLevel[2] = blackLevelTag[2];
                        break;
                    case GBRG:
                        blackLevel[0] = blackLevelTag[2];
                        blackLevel[1] = (blackLevelTag[0] + blackLevelTag[3])/2;
                        blackLevel[2] = blackLevelTag[1];
                        break;
                    default:
                        fatalError("loadDNG: %s: Unexpected Bayer value (should already have been validated)!", filename.c_str());
                };
            } else {
                fatalError("loadDNG: %s: Unexpected type of black level tag found", filename.c_str());
            }
        }
        dprintf(4,"loadDNG: %s: Black levels are %d, %d, %d\n", filename.c_str(), blackLevel[0], blackLevel[1], blackLevel[2]);

        // Read in white level
        entry = rawIfd->find(DNG_TAG_WhiteLevel);
        uint16_t whiteLevel = 65535;
        if (entry) {
            whiteLevel = (int)entry->value();
        }
        dprintf(4,"loadDNG: %s: White level is %d\n", filename.c_str(), whiteLevel);

        //
        // Let's find the Thumbnail IFD, if any
        TiffIfd *thumbIfd = NULL;

        entry = dng.ifds(0)->find(TIFF_TAG_NewSubFileType);
        ifdType = TIFF_NewSubfileType_DEFAULT;
        if (entry) {
            ifdType = entry->value();
        }

        if (ifdType == (int)TIFF_NewSubfileType_MainPreview) {
            // Main IFD has thumbnail data
            dprintf(4, "loadDNG: Thumbnail data found in IFD0\n");
            thumbIfd = dng.ifds(0);
        } else {
            // Search subIFDs for thumbnail data
            for (size_t i=0; i < dng.ifds(0)->subIfds().size(); i++) {
                ifdType = TIFF_NewSubfileType_DEFAULT;
                entry = dng.ifds(0)->subIfds(i)->find(TIFF_TAG_NewSubFileType);
                if (entry) {
                    ifdType = entry->value();
                }
                if (ifdType == (int)TIFF_NewSubfileType_MainPreview) {
                    dprintf(4, "loadDNG: Thumbnail data found in subIFD %d\n", i);
                    thumbIfd = dng.ifds(0)->subIfds(i);
                    break;
                }
            }            
        }

        if (thumbIfd) {
            _f->thumbnail = thumbIfd->getImage();
        }

        //
        // Let's grab other useful metadata from the main IFD

        // Verify orientation
        entry = dng.ifds(0)->find(TIFF_TAG_Orientation);
        if (!entry) fatalError("loadDNG: %s: No orientation entry found", filename.c_str());
        int orientation = entry->value();

        if (orientation != TIFF_Orientation_TopLeft) fatalError("loadDNG: %s: Unsupported orientation value %d", filename.c_str(), orientation);

        // Read in color calibration information
        entry = dng.ifds(0)->find(DNG_TAG_CalibrationIlluminant1);
        unsigned int calibIlluminant1 = 0;
        if (entry) calibIlluminant1 = (int)entry->value();
        if (calibIlluminant1 >= DNG_CalibrationIlluminant_Values) calibIlluminant1 = DNG_CalibrationIlluminant_Values - 1;

        int calibTemp1 = DNG_CalibrationIlluminant_Temp[calibIlluminant1];
        dprintf(4,"loadDNG: %s: Calibration illuminant #1 is type %d (%d K)\n", filename.c_str(), calibIlluminant1, calibTemp1);

        std::vector<float> camToRGBMatrix1(9);
        std::vector<float> rgbBlackLevel1(3);
        {
            entry = dng.ifds(0)->find(DNG_TAG_ColorMatrix1);
            if (!entry) fatalError("loadDNG: %s: No color calibration matrix found in IFD0", filename.c_str());

            std::vector<double> &xyzToCamMatrix1 = entry->value();

            std::vector<float> rgbToCamMatrix1(9);

            for (int i=0;i<3;i++) {
                for (int j=0; j<3; j++) {
                    for (int k=0; k<3;k++) {
                        rgbToCamMatrix1[i*3+j] += RGBtoXYZ[i*3+k]*xyzToCamMatrix1[k*3+j];
                    }
                }
            }
            invert3x3(&rgbToCamMatrix1[0], &camToRGBMatrix1[0]);
            for (int i=0;i<3;i++) {
                for (int j=0; j<3; j++) {
                    rgbBlackLevel1[i] += camToRGBMatrix1[i*3+j]*(-blackLevel[j]);
                }
            }
        }

        entry = dng.ifds(0)->find(DNG_TAG_CalibrationIlluminant2);
        int numCalibIlluminants = 1;
        int calibTemp2 = -1;
        std::vector<float> camToRGBMatrix2(9);
        std::vector<float> rgbBlackLevel2(3);
        if (entry) {
            numCalibIlluminants = 2;
            unsigned int calibIlluminant2 = (int)entry->value();
            if (calibIlluminant2 >= DNG_CalibrationIlluminant_Values) calibIlluminant2 = DNG_CalibrationIlluminant_Values - 1;

            calibTemp2 = DNG_CalibrationIlluminant_Temp[calibIlluminant2];
            dprintf(4,"loadDNG: %s: Calibration illuminant #2 is type %d (%d K)\n", filename.c_str(), calibIlluminant2, calibTemp2);

            entry = dng.ifds(0)->find(DNG_TAG_ColorMatrix2);
            if (!entry) fatalError("loadDNG: %s: No color matrix found for second calibration illuminant", filename.c_str());

            std::vector<double> &xyzToCamMatrix2 = entry->value();

            std::vector<float> rgbToCamMatrix2(9);
            for (int i=0;i<3;i++) {
                for (int j=0; j<3; j++) {
                    for (int k=0; k<3;k++) {
                        rgbToCamMatrix2[i*3+j] += RGBtoXYZ[i*3+k]*xyzToCamMatrix2[k*3+j];
                    }
                }
            }
            invert3x3(&rgbToCamMatrix2[0], &camToRGBMatrix2[0]);
            for (int i=0;i<3;i++) {
                for (int j=0; j<3; j++) {
                    rgbBlackLevel2[i] += camToRGBMatrix2[i*3+j]*(-blackLevel[j]);
                }
            }

        }

        // Read in camera model
        entry = dng.ifds(0)->find(TIFF_TAG_Model);
        std::string cameraModel = "Unknown";
        if (entry) cameraModel = (std::string)entry->value();
        dprintf(4,"loadDNG: %s: Camera model is %s\n", filename.c_str(), cameraModel.c_str());

        // Read in camera manufacturer
        entry = dng.ifds(0)->find(TIFF_TAG_Make);
        std::string cameraManufacturer = "Unknown";
        if (entry) cameraManufacturer = (std::string)entry->value();
        dprintf(4,"loadDNG: %s: Camera manufacturer is %s\n", filename.c_str(), cameraManufacturer.c_str());

#undef fatalError

        // Write values that might be overriden by private data
        _f->dng.minRawValue = std::min(blackLevel[0], std::min(blackLevel[1], blackLevel[2]));
        _f->dng.maxRawValue = whiteLevel;
        _f->dng.numIlluminants = numCalibIlluminants;
        _f->dng.illuminant1 = calibTemp1;
        _f->dng.illuminant2 = calibTemp2;
        for (int i=0; i < 3; i++) {
            for (int j=0; j < 3; j++) {
                _f->dng.colorMatrix1[i*4+j] = camToRGBMatrix1[i*3+j];
                _f->dng.colorMatrix2[i*4+j] = camToRGBMatrix2[i*3+j];
            }
        }
        for (int i=0; i < 3; i++) {
            _f->dng.colorMatrix1[i*4+3] = rgbBlackLevel1[i];
            _f->dng.colorMatrix2[i*4+3] = rgbBlackLevel2[i];
        }

        // Grab our private DNG data to supplement/replace the above
        entry = dng.ifds(0)->find(DNG_TAG_DNGPrivateData);
        if (entry) {
            std::string &privateString = entry->value();
            // Extract preamble string
            int preambleEnd = privateString.find((char)0);
            std::string preamble = privateString.substr(0,preambleEnd);
            if (preamble == privateDataPreamble) {
                // Extract the private data, starting with version
                std::stringstream privateData(privateString.substr(preambleEnd+1));
                TagValue version;
                privateData >> version;
                
                if (version.asInt() == 1) {
                    dprintf(4,"loadDNG: %s: Reading private data, version 1.\n", filename.c_str());
                    loadDNGPrivateData_v1(_f, privateData);
                } else {
                    TagValue backwardVersion;
                    privateData >> backwardVersion;
                    switch (backwardVersion.asInt()) {
                    case 2:
                        dprintf(4,"loadDNG: %s: Reading private data, version 2.\n", filename.c_str());
                        loadDNGPrivateData_v2(_f, privateData);
                        break;                        
                    default:
                        warning(Event::FileLoadError,
                                "loadDNG: %s: Private data version too new: %d,%d (can handle X, %d), ignoring it.",
                                filename.c_str(), version.asInt(), backwardVersion.asInt(), privateDataVersion);
                    }
                }
            } else {
                warning(Event::FileLoadError,
                        "loadDNG: %s: Private data preamble doesn't match FCam's: '%s' (expecting '%s'), ignoring it.",
                        filename.c_str(), preamble.c_str(), privateDataPreamble);
            }
        }
        // Write remaining frame information

        _f->dng.bayerPattern = bayer;
        _f->dng.manufacturer = cameraManufacturer;
        _f->dng.model = cameraModel;

        dprintf(4,"loadDNG: %s: Loaded successfully\n", filename.c_str());
        return f;
    }

}
