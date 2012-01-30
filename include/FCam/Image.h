#ifndef FCAM_IMAGE_H
#define FCAM_IMAGE_H

#include "Base.h"
#include <stdio.h>
#include <pthread.h>
#include <string>

/** \file
 * FCam Image objects. */

namespace FCam {

    /** A reference-counted Image object.
     *
     * Images are stored in row-major order, with the origin is the
     * top left corner of the image. While each scanline is guaranteed
     * to be consecutive in memory, there may be padding in between
     * adjacent scanlines.
     *
     * If you instantiate an image using one of the constructors that
     * allocates data, the result will be a reference counted image
     * object, that automatically deletes the data when the last
     * reference to it is destroyed.
     * 
     * If you use a constructor that sets the data field, you're
     * telling the image class that someone else is managing that
     * memory, and the destructor will never be called.
     *
     * Either way, you can safely pass Image objects by value without
     * incurring the cost of memory copies. Only the explicit \c copy
     * methods will copy the underlying image data.
     */
    class Image {
    public:
        /** @name Image property accessors
         * 
         * These methods return values of the various Image
         * properties, which are immutable for a given Image instance.
         */
        
        //@{

        /** The size of the image in pixels.  Note: The image size in
         * memory is not necessarily equal to
         * \c size.width*size.height*bytesPerPixel, since \a bytesPerRow may
         * be larger than \c size.width*bytesPerPixel if dealing with
         * aligned images or subimages.  In addition for planar images, 
         * bytesPerPixel == 1 and planes are stored at different offsets.
         * The true image memory extent is \c allocateHeight()*bytesPerRow.
         */
        inline Size size() const {
            return _size;
        }

        /** The width of the image in pixels. */
        inline unsigned int width() const {
            return _size.width;
        }

        /** The height of the image in pixels. */
        inline unsigned int height() const {
            return _size.height;
        }

        /** The actuall allocated image height - 
          * For planar images this is adjusted to fit all planes
          * since in this case bytesPerPixel == 1. */
        unsigned int allocateHeight() const;


        /** The format of the image data. 
         */
        inline ImageFormat type() const {
            return _type;
        }

        /** The size of an image pixel in bytes.
         *
         * This is strictly equal to \a FCam::bytesPerPixel(type()), and is
         * cached here for convenience
         */
        inline unsigned int bytesPerPixel() const {
            return _bytesPerPixel;
        }

        /** The increment between two rows of the image in bytes
         *
         * This is the number of bytes you have to increment by to
         * move from one row to the next.  That is, pixel \c (x,y) is
         * located at memory location \c data+y*bytesPerRow+x*bytesPerPixel.
         * \a bytesPerRow is at least equal to
         * \c size.width*bytesPerPixel, but may be larger for a subimage
         * (an image that's only a region of a larger source image),
         * or an image that has rows aligned in memory for faster
         * access.
         */
        inline unsigned int bytesPerRow() const {
            return _bytesPerRow;
        }

        /** Test if it's safe to access the image data. 
         */
        inline bool valid() const {
            return (data != Image::Discard && data != Image::AutoAllocate);
        }

        /** Test if this image is a special Discard image.  When
         * passed as an argument to a function, a Discard image
         * indicates that the function should not produce any image
         * output, but should perform all its other tasks. Passing a
         * Shot object with a Discard image to a Sensor results in a
         * Frame with a non-valid Image, but all device actions are
         * performed, and all Frame metadata, such as histogram data
         * or Device tags, are valid. You can create a Discard image with:
         * \c Image(size, type, Image::Discard)
         */
        inline bool discard() const {
            return data == Image::Discard;
        }

        /** Test if this image is a special AutoAllocate image.  When
         * passed as an argument to a function, an AutoAllocate image
         * indicates that the function should create a new Image to
         * store the results in, based on the attributes of the Image
         * passed in.  Passing a Shot object with an AutoAllocate
         * image to a Sensor makes the Sensor allocate a new buffer
         * for the captured image data, which is returned in the
         * Frame. This allows you to reuse the same Shot object for
         * multiple capture requests (or a stream request) without
         * getFrame() overwriting the data for an earlier Frame
         * image. You can create an AutoAllocate image with:
         * \c Image(size, type, Image::AutoAllocate)
         */
        inline bool autoAllocate() const {
            return data == Image::AutoAllocate;
        }

        //@}

        /** This value for data represents an image with no allocated
         * memory that should never have data copied into it. It is
         * equal to NULL.
         */
        static unsigned char *Discard;

        /** This value for data represents image data whose first user
         * should allocate memory for. If used as the image field of
         * a Shot, it indicates that a new buffer should be allocated
         * for every new frame.
         */
        static unsigned char *AutoAllocate;

        ~Image();

        /** @name Allocating Constructors and Factory Methods
         * 
         * These image constructors allocate new memory for image
         * data. */
        
        //@{
        /** Construct a new image of the given size in the given format. */
        Image(Size, ImageFormat);
        Image(int, int, ImageFormat);

	/** Use memory mapped I/O to construct an image from a buffer
	 * stored in a file.  The image will therefore be paged out as
	 * needed, and if the optional parameter is set to true,
	 * modifying the image data will also modify the image on
	 * disk. Note that the file descriptor passed to this
	 * constructor can be closed after the Image is constructed
	 * with no problems, and the file can even be deleted - in
	 * that case, the file will not actually be erased until the
	 * last Image referring to the buffer is destroyed. The
	 * arguments are the file description of the file, the number
	 * of bytes into the file the image buffer starts at, and the
	 * dimensions and format of the image, followed by whether
	 * changing the image data also changes the file. */
	Image(int fd, int offset, Size, ImageFormat, bool writeThrough = false);

        /** Returns an Image containing a copy of the current Image's
         * data. Useful for converting an Image with a weak reference
         * to an Image with a strong reference to its data.  This
         * requires a full copy of the underlying image data, so it
         * can be quite slow for large images. 
         */
        Image copy() const;

        //@}

        /** @name Non-allocating Constructors and Factory Methods
         *
         * These image constructors make a new reference to already
         * existing image data. */
        //@{

        /** Construct a new image of the given size in the given
         * format using already existing data. The image will never
         * deallocate this data. This is a good way to cast some other
         * image type to FCam's image type without allocating new data
         * or copying image data. These constructors optionally take
         * in the number of bytes per row of image data, to allow for
         * Images that contain a subset of the source data, or for
         * source data with padding.
         */
        Image(Size, ImageFormat, unsigned char *, int srcBytesPerRow=-1);
        Image(int, int, ImageFormat, unsigned char *, int srcBytesPerRow=-1);

        /** Construct a new image with no memory allocated */
        Image();

        /** Make a copy of the image reference. Doesn't copy image
         * data, just produces a new reference to the same data.
         */
        Image(const Image &other); 

        /** Retuns an image that points to a section of the current
         *  image. The subimage shares the same underlying buffer.
         *  The requested size will be trimmed if it would exceed the
         *  bounds of the current image. The arguments are the
         *  location of the top-left corner of the subImage and the
         *  size of the subImage.
         */
        Image subImage(unsigned int x, unsigned int y, Size) const;

        /** Become a new reference to an existing image. This never
         * copies image data. It just produces a new reference to the
         * same data, with the same properties. 
         */
        const Image &operator=(const Image &other);

        //@}

        /** @name Image data accessors
         * 
         * These methods allow you to change the pixel data referenced by the Image.
         */
        
        //@{

        /** Access a pixel in the image. Returns a pointer to the
         * first byte of a pixel at image location \a (x,y).  The image
         * is laid out in memory in row-major order. Each row is
         * contigious, and there may be a gap between rows, with each
         * row \a y of pixels starting at memory offset
         * <tt> y * bytesPerRow() </tt>. Each row itself is 
         * <tt> width() * bytesPerPixel() </tt> bytes long.
         */
        inline unsigned char *operator()(unsigned int x, unsigned int y) const {
            return data + bytesPerPixel()*x + bytesPerRow()*y;
        }

        /** Copies the image data from the source Image into this
         *  Image.  Useful for transferring image blocks into
         *  pre-allocated (possibly weakly-referenced) buffers.
         *  Mismatched sizes are allowed. In case of a larger source
         *  image, only the section that fits into this image will be
         *  copied. In case of a smaller source image, the pixels in
         *  this Image outside the source image's area are not
         *  modified. Ignores image types entirely. Actually copies
         *  image data, so can be quite slow.
         */
        void copyFrom(const Image &);
        //@}

        /** @name Miscellaneous methods
         */ 

        /** Lock the image to prevent other threads writing to it with
         * a configurable timeout in microseconds.
         *
         * Returns whether the lock was acquired.
         *
         * The default timeout (-1) indicates you should wait
         * indefinitely, and should always return true. A timeout of
         * zero will not block at all.
         *
         * Images have locks to coordinate access between multiple
         * threads. When new frames come in, the FCam API will try to
         * lock the target image for them with a short timeout and
         * copy the data in. If the image is still locked at the end
         * of the timeout, it will instead drop the data on the floor
         * (avoiding the expensive memcpy). You can use this for rate
         * control of a viewfinder like so:

         \verbatim
         Shot viewfinder;
         ...
         viewfinder.image = Image(640, 480, UYUV);
         ...
         sensor.stream(viewfinder);
         while (1) {
             // Get the most recent viewfinder frame with image data
             Frame f;
             do {
                 f = sensor.getFrame();
                 // feed the frame to the autoexposure and autofocus routines
                 ...
             } while (sensor.framesPending() || !f.image().valid());
           
             f.image().lock();
             // Do something expensive with the image. 
             // No memcpys into the image will take place during this time.
             f.image().unlock();
         }        
         \endverbatim
         
         * If you absolutely want to save every piece of image data,
         * you should set the target image to an AutoAllocated one,
         * Even the example above doesn't guarantee image data hasn't
         * been clobbered just before you call lock(). It only
         * guarantees you get one consistent image and not a
         * half-written one. If you spend all your CPU time with the
         * frame locked, you may starve the FCam runtime of chances to
         * give you new image data.
         *
         * Your image may still get clobbered if other threads don't
         * respect the lock, or if the image is a weak reference, and
         * other independently created weak references to the same
         * data exist are being used to write to it. Best to only have
         * one image that refers to external data (images created by
         * other libraries, framebuffers, etc), and construct other
         * references to that data using copies of the first image.
         */
        bool lock(int timeout = -1);

        /** Unlock the image. See \ref lock for details. */
        void unlock();

        /** Compare two images for equality. Two images are equal if
         * they view the same image data with the same attributes, so
         * two offset subimages of a larger image won't be equal, and
         * neither would two subimages of the same region, if one has
         * a different type than the other.
         */
        bool operator==(const Image &) const;

        /** Is this a weak reference to somebody else's data? */
        bool weak();

        /** Print out some debugging info about the image, optionally
         * taking in a name to identify the image.
         */
        void debug(const char *name="") const;

        /** Store extra private date with the image.
          * This is for FCam components to hold information
          * on hw accelerated formats such as overlays/textures. */
        void setPrivateData(void *prv) {
            privateData = prv;
        }

        /** Retrieve the private data. */
        void *getPrivateData() {
            return privateData;
        }

        //@}
    private:
        Size _size;
        ImageFormat _type;
        unsigned int _bytesPerPixel;
        unsigned int _bytesPerRow;

        /** A pointer to the start of image data inside the image
         * buffer. The image is laid out in memory in row-major order,
         * with each row \a y of pixels starting at memory location 
         * \c data+y*bytesPerRow. Each row itself is 
         * \c size.width*bytesPerPixel bytes long.
         */
        unsigned char *data;
        
        /** A pointer to the start of the memory buffer storing the
         * image data.  In case of windowed images, this is not equal
         * to data.
         */
        unsigned char *buffer;
        unsigned int bytesAllocated;

        // Reference counting mechanisms
        unsigned int *refCount;
        pthread_mutex_t *mutex; 

	// Is this a memory mapped image?
	bool memMapped;

        // Does this reference currently have the image locked?
        bool holdingLock;

        // A pointer to the private data;
        void *privateData;

        /** Make the image refer to data stored elsewhere. Internally
         *  used by the constructors to centralize some common operations
         */         
        void setBuffer(unsigned char *b, unsigned char *d=NULL);

    };

}

/** A debug helper macro that outputs the variable name of the image
 * object at the macro call site along with the image debug
 * information */
#define FCAM_IMAGE_DEBUG(i) (i).debug(#i)

#endif

