#ifndef FCAM_HISTOGRAM_H
#define FCAM_HISTOGRAM_H

#include <vector>

/*! \file
 * The Histogram and HistogramConfig classes
 */

namespace FCam {


    /** The configuration of the histogram generator. */
    class HistogramConfig {
    public:
        /** The default constructor disables the histogram
         * generator. */
        HistogramConfig() : buckets(64), enabled(false) {}

        /** Rectangle specifying the region over which a histogram is
         * computed. */
        Rect region;

        /** The requested number of buckets in the histogram. The N900
         * implementation ignores this request and gives you a
         * 64-bucket histogram. */
        unsigned buckets;

        /** Whether or not a histogram should be generated at all */
        bool enabled;

        /** Compare two requested configurations to see if they would
         * return the same data. */
        bool operator==(const HistogramConfig &other) const {
            if (enabled != other.enabled) return false;
            if (buckets != other.buckets) return false;
            if (region != other.region) return false;
            return true;
        }

        /** Compare two requested configurations to see if they would
         * return the same data. */
        bool operator!=(const HistogramConfig &other) const {
            return !((*this) == other);
        }
    };


    /** A histogram returned by the histogram generator. Before you
     * dereference the data, check if valid is true. Even if you
     * requested a histogram, you're not guaranteed to get one back in
     * all cases. */
    class Histogram {
        unsigned _buckets, _channels;
        Rect _region;
        std::vector<unsigned> _data;
        ColorSpace _colorspace;
    public:

        /** The default constructor is an invalid histogram storing no
         * data in RGB colorspace. */
        Histogram(): _buckets(0), _channels(0), _region(0, 0, 0, 0), _colorspace(RGB) {
        }

        /** Make a new empty histogram of the given number of buckets and channels, using the given 
          * colorspace (defaults to RGB if not specified). */
        Histogram(unsigned buckets, unsigned channels, Rect region, ColorSpace colorspace = RGB) :
            _buckets(buckets), _channels(channels), _region(region), _colorspace(colorspace) {
            _data.resize(buckets*channels);
        }

        /** Sample the histogram at a particular bucket in a
         * particular image channel. The absolute number should not be
         * relied on because the histogram could be computed over the
         * bayer-mosaiced raw sensor data or subsampled.
         * If color histograms are available, check the color space for the
         * order and definition of the channels. If the color space is
         * RGB, which will be in the  sensor's raw color space,
         * the green channel is normalized to have roughly the same total
         * count as red and blue, despite there being twice as many
         * green samples on the raw sensor. */
        unsigned operator()(int b, int c) const {
            return _data[b*_channels + c];
        }

        /** Acquire a reference to a particular histogram
         * entry. Useful for creating your own histograms. */
        unsigned &operator()(int bucket, int channel) {
            return _data[bucket*_channels + channel];
        }

        /** Sample the histogram at a particular bucket, summed over
         * all channels for RGB color spaces, return Y for YUV color spaces */
        unsigned operator()(int bucket) const {
            unsigned result = 0;

            switch (_colorspace) {
                case RGB:
                case sRGB:
                    for (size_t channel = 0; channel < _channels; channel++)
                        result += (*this)(bucket, channel);
                    break;
                case YUV:
                case YpUV:
                    result = (*this)(bucket,0);
                    break;
                default:
                    result = 0;
            }
            return result;
        }

        /** Is it safe to dereference data and/or call operator()? */
        bool valid() const {
            return _data.size() != 0;
        }

        /** Access to the raw histogram data. Stored similarly to an
         * image: first bucket, then color channels. */
        unsigned *data() {return &_data[0];}

        /** Returns the number of buckets in the histogram. */
        unsigned buckets() const {return _buckets;}

        /** Returns the number of channels in the histogram. */
        unsigned channels() const {return _channels;}

        /** Returns the region of the image that this histogram was computed over. */
        Rect region() const {return _region;}

        /** Returns the colorspace of the image the histogram was computed on. 
          * Depending on the processing stage in the image pipeline the histogram is 
          * computed, the colorspace could be RAW RGB, sRGB, YUV, YpUV (gamma corrected). */
        ColorSpace colorspace() const {return _colorspace; }
    };

}

#endif
