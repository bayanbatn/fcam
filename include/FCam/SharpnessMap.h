#ifndef FCAM_SHARPNESS_MAP_H
#define FCAM_SHARPNESS_MAP_H

#include <vector>

/*! \file
 * The SharpnessMap and SharpnessMapConfig classes
 */  

namespace FCam {
    /** The configuration of the sharpness map generator. A sharpness
     * map of a given resolution can be computed. Currently this is
     * computed over the entire image. */
    class SharpnessMapConfig {
      public:
        /** The default constructor disables the sharpness map
         * generator */
        SharpnessMapConfig() : size(0, 0), enabled(false) {}

        /** The requested sharpness map resolution. Currently on the
         * N900 this request is ignored and you always get a 16x12
         * sharpness map. */
        Size size;

        /** Whether or not a sharpness map should be generated. */
        bool enabled;

        /** Compare two requested configurations to see if they would
         * return the same data. */
        bool operator==(const SharpnessMapConfig &other) const {
            if (enabled != other.enabled) return false;
            if (enabled && size != other.size) return false;
            return true;
        }

        /** Compare two requested configurations to see if they would
         * return the same data. */
        bool operator!=(const SharpnessMapConfig &other) const {
            return !((*this) == other);
        }
    };    


    /** A sharpness map returned by the sharpness map generator. The
     * sharpness map is the absolute value of a high-pass IIR filter
     * summed over each region. */
    class SharpnessMap {
    private:
        Size _size;
        unsigned _channels;
        std::vector<unsigned> _data;

    public:

        /** The default sharpnes map carries no data. */
        SharpnessMap() : _size(0, 0), _channels(0) {
        }

        /** Make an empty sharpness map of the given size and channel count */
        SharpnessMap(Size s, int channels) : _size(s), _channels(channels) {
            _data.resize(s.width*s.height*channels);            
        }

        /** Return sharpness at a particular location in a particular
         * channel. The order of the channels is RGB in the sensor's
         * raw color space. The absolute sharpness carries only
         * relative meaning, and depends on the particular
         * sharpness-detecting filter used by the implementation. Be
         * aware that for linear filters, brighter regions will have a
         * higher response due to Poisson noise. These numbers can be
         * quite large, so if you're summing up the sharpness map, you
         * should take care to prevent overflow. */
        unsigned operator()(int x, int y, int c) const {
            return _data[(y*_size.width+x)*_channels + c];
        }

        /** Return a reference into the sharpness map. This is useful
         * if you want to generate your own fake sharpness maps. */
        unsigned &operator()(int x, int y, int c) {
            return _data[(y*_size.width+x)*_channels + c];
        }

        /** Return sharpness at a particular location in the sharpness
         * map summed over all channels. */
        unsigned operator()(int x, int y) const {
            unsigned total = 0;
            for (size_t c = 0; c < _channels; c++) {
                total += (*this)(x, y, c);
            }
            return total;
        }       

        /** Is it safe to dereference data and/or call operator(). */
        bool valid() const {return _data.size() != 0;}

        /** How many channels are there in the sharpness map. Typically this is 3. */
        unsigned channels() const {return _channels;}

        /** What resolution is the sharpness map */
        const Size &size() const {return _size;} 

        /** How many rows does the sharpness map have */
        int height() const {return _size.height;}

        /** How many columns does the sharpness map have */
        int width() const {return _size.width;}

        /** The raw sharpness data. Stored in a similar order to an
         * image: rows, then columns, then channels. */
        unsigned *data() {return &_data[0];}
    };
}

#endif
