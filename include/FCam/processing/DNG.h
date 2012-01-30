#ifndef FCAM_DNG_H
#define FCAM_DNG_H

#include "../Frame.h"
#include <string>

/** \file 
 * Loading and saving DNG files. All metadata, including all
 * frame tags, are properly saved and loaded. Only FCam-created DNGs can
 * be loaded.
 */

namespace FCam {

    /** TiffFile is a representation of the internals of a TIFF file, which a DNG file also is. */        
    class TiffFile;
    class TiffIfd;

    /** A _Frame struct with added custom fields to encode DNG fields
     * and a DNG thumbnail.  You should not instantiate a _DNGFrame
     * directly, unless you're making dummy frames for testing
     * purposes. */ 
    class _DNGFrame : public _Frame, public FCam::Platform {
    public:
        _DNGFrame();
        ~_DNGFrame();

        /** DNGFrames also act as their own platform data,
         * because those properties may vary per DNG. */
        const Platform &platform() const {return *this;}

        Shot _shot;

        TiffFile *dngFile;

        Image thumbnail;

        // LoadDNG should set all these
        struct {
            BayerPattern bayerPattern;
            unsigned short minRawValue, maxRawValue;
            int numIlluminants;
            float colorMatrix1[12], colorMatrix2[12];
            int illuminant1, illuminant2;
            std::string manufacturer, model;
        } dng;

        const Shot &shot() const { return _shot; }
        const Shot &baseShot() const { return shot(); }

        BayerPattern bayerPattern() const {return dng.bayerPattern;}
        unsigned short minRawValue() const {return dng.minRawValue;}
        unsigned short maxRawValue() const {return dng.maxRawValue;}
        void rawToRGBColorMatrix(int kelvin, float *matrix) const;

        const std::string &manufacturer() const {return dng.manufacturer;}
        const std::string &model() const {return dng.model;}

        /** A debugging dump function. Prints out all
         * DNGFrame-specific fields, and then calls
         * FCam::_Frame::debug to print the base fields and tags.
         */
        virtual void debug(const char *name="") const;
    };

    /** A DNGFrame is constructed by loadDNG, and contains all the
     * metadata found in a DNG file.  The DNGPrivateData field is used
     * to losslessly store all FCam::Frame fields and the TagMap.  If
     * a thumbnail is stored in the DNG file, it can be retrieved
     * using the thumbnail method.
     */
    class DNGFrame: public FCam::Frame {
    protected:
        _DNGFrame *get() const { 
            return static_cast<_DNGFrame*>(ptr.get()); 
        }

    public:

        /** DNGFrames are normally created by loadDNG. The
         * Frame constructor can be used to construct dummy DNG frames for
         * testing purposes. The DNGFrame takes ownership of the _DNGFrame
         * passed in. */
        DNGFrame(_DNGFrame *f=NULL);
  
        Image thumbnail();
    };

    /** Save a DNG file. The frame must have an image in RAW format.
     * All FCam::Frame fields and the tag map are saved in the DNG, along with
     * a thumbnail of the image.
     */
    void saveDNG(Frame frame, const std::string &filename);
    /** Load a DNG file. Only DNG files saved by FCam are properly supported.
     */
    DNGFrame loadDNG(const std::string &filename);
}

#endif
