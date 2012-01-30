//#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <algorithm>

#include "FCam/Image.h"
#include "FCam/Time.h"
#include "FCam/Event.h"
#include "Debug.h"

namespace FCam {

    unsigned char *Image::Discard = (unsigned char *)(0);
    unsigned char *Image::AutoAllocate = (unsigned char *)(-1);

    Image::Image()
        : _size(0, 0), _type(UNKNOWN), _bytesPerPixel(0), _bytesPerRow(0), 
          data(Image::Discard), buffer(NULL), bytesAllocated(0),
          refCount(NULL), mutex(NULL), 
          memMapped(false), holdingLock(false), 
          privateData(NULL) {
    }
    
    Image::Image(int w, int h, ImageFormat f) 
        : _size(w, h), 
          _type(f), 
          _bytesPerPixel(FCam::bytesPerPixel(f)), 
          _bytesPerRow(bytesPerPixel()*width()),
          data(NULL), buffer(NULL), bytesAllocated(0),
          refCount(NULL), mutex(NULL), 
          memMapped(false),
          holdingLock(false),
          privateData(NULL) {
        
        bytesAllocated = bytesPerRow()*allocateHeight();
        setBuffer(new unsigned char[bytesAllocated]);
        refCount = new unsigned;
        *refCount = 1; // only I know about this data
        mutex = new pthread_mutex_t;
        pthread_mutex_init(mutex, NULL);
    }
    
    Image::Image(Size s, ImageFormat f) 
        : _size(s), 
          _type(f), 
          _bytesPerPixel(FCam::bytesPerPixel(f)), 
          _bytesPerRow(bytesPerPixel()*width()),
          data(NULL), buffer(NULL), bytesAllocated(0),
          refCount(NULL), mutex(NULL), 
          memMapped(false),
          holdingLock(false),
          privateData(NULL) {

        bytesAllocated = bytesPerRow()*allocateHeight();
        setBuffer(new unsigned char[bytesAllocated]);        
        refCount = new unsigned;
        *refCount = 1; // only I know about this data
        mutex = new pthread_mutex_t;
        pthread_mutex_init(mutex, NULL);
    }

    Image::Image(int fd, int offset, Size s, ImageFormat f, bool writeThrough) 
        : _size(s), 
          _type(f), 
          _bytesPerPixel(FCam::bytesPerPixel(f)),
          _bytesPerRow(bytesPerPixel()*width()),
          data(NULL), buffer(NULL), bytesAllocated(0),
          refCount(NULL), mutex(NULL),
          memMapped(true),
          holdingLock(false),
          privateData(NULL) {
        
        unsigned char *mappedBuffer;
        int flags;
        if (writeThrough) {
            flags = MAP_SHARED;
        } else {
            flags = MAP_PRIVATE;
        }
        // Make starting offset a multiple of page size, and determine relative offset to true buffer start
        int pageSize = getpagesize();
        int startOfMap = (offset/pageSize)*pageSize;
        int mapOffset = offset-startOfMap;
        // Make mapping size a multiple of page size, rounding up
        int bytesToMap = bytesPerRow()*height()+mapOffset; 
        bytesAllocated = ((bytesToMap-1)/pageSize+1) *pageSize;
        dprintf(5, 
                "Image::Image(): Mapping image from file %d. "
                "Requsted start %x, length %x. "
                "Actual start: %x, offset %x, length %x\n", 
                fd, offset, bytesPerRow()*height(), 
                startOfMap, offset, bytesAllocated);

        mappedBuffer = (unsigned char*)mmap(NULL, 
                                            bytesAllocated,
                                            PROT_READ | PROT_WRITE,
                                            flags,
                                            fd,
                                            startOfMap);

        if (mappedBuffer == MAP_FAILED) {
            error(Event::InternalError, 
                  "Image: Unable to memory map file descriptor %d at %d, length %d bytes: %s",
                  fd, offset, bytesPerRow()*height(), strerror(errno)
                  );
            return;
        }

        int success;
        // Will almost always do sequential access to images stored on disk, so let's let the OS know.
#ifdef FCAM_PLATFORM_CYGWIN
        // This is returning an error currently, so leaving it out for now
        // Not much of a performance problem on desktop machines anyway
        // success = posix_madvise(mappedBuffer, bytesAllocated, POSIX_MADV_SEQUENTIAL);
        // if (success != 0) {
        //    warning(Event::InternalError,
        //            "Image: Unable to call madvise successfully. Performance may be impacted: %s", strerror(success));
        //}
        success =0;
#else
        success = madvise(mappedBuffer, bytesAllocated, MADV_SEQUENTIAL);
        if (success != 0) {
            warning(Event::InternalError,
                    "Image: Unable to call madvise successfully. Performance may be impacted: %s", strerror(errno));
        }
#endif

        setBuffer(mappedBuffer, mappedBuffer+mapOffset);
        refCount = new unsigned;
        *refCount = 1; // only I know about this data
        mutex = new pthread_mutex_t;
        pthread_mutex_init(mutex, NULL);
    }

    Image::Image(Size s, ImageFormat f, unsigned char *d, int srcBytesPerRow) 
        : _size(s), 
          _type(f), 
          _bytesPerPixel(FCam::bytesPerPixel(f)), 
          data(NULL), buffer(NULL), bytesAllocated(0),
          refCount(NULL), mutex(NULL), 
          memMapped(false),
          holdingLock(false), 
          privateData(NULL) {

        _bytesPerRow = (srcBytesPerRow == -1) ? (bytesPerPixel() * width()) : srcBytesPerRow;
        setBuffer(NULL, d);

        if (valid()) {
            refCount = new unsigned;
            *refCount = 2; // there's me, and then there's the real owner of this data
            mutex = new pthread_mutex_t;
            pthread_mutex_init(mutex, NULL);
        }
    }
    
    Image::Image(int w, int h, ImageFormat f, unsigned char *d, int srcBytesPerRow) 
        : _size(w, h), 
          _type(f), 
          _bytesPerPixel(FCam::bytesPerPixel(f)),
          data(NULL), buffer(NULL), bytesAllocated(0),
          refCount(NULL), mutex(NULL), 
          memMapped(false),
          holdingLock(false),
          privateData(NULL) {

        _bytesPerRow = (srcBytesPerRow == -1) ? (bytesPerPixel() * width()) : srcBytesPerRow;
        setBuffer(NULL, d);

        if (valid()) {
            refCount = new unsigned;
            *refCount = 2; // there's me, and then there's the real owner of this data
            mutex = new pthread_mutex_t;
            pthread_mutex_init(mutex, NULL);
        }
    }

    Image::~Image() {
        setBuffer(NULL);        
    }

    Image::Image(const Image &other) 
        : _size(other.size()), 
          _type(other.type()), 
          _bytesPerPixel(other.bytesPerPixel()),
          _bytesPerRow(other.bytesPerRow()),
          data(other.data), buffer(other.buffer),
          bytesAllocated(other.bytesAllocated),
          refCount(other.refCount),
          mutex(other.mutex), 
          memMapped(other.memMapped), 
          holdingLock(false),
          privateData(other.privateData) {
        if (refCount) {
            (*refCount)++;
        }
    };

    const Image &Image::operator=(const Image &other) {
        if (this == &other) return (*this);
        if (refCount && 
            refCount == other.refCount &&
            data == other.data) {
            return (*this);
        }

        _size = other.size();
        _type = other.type();
        _bytesPerPixel = other.bytesPerPixel();
        _bytesPerRow = other.bytesPerRow();
        setBuffer(other.buffer, other.data);
        bytesAllocated = other.bytesAllocated;
        
        refCount = other.refCount;
        mutex = other.mutex;
        if (refCount) (*refCount)++;
        memMapped = other.memMapped;
        holdingLock = false;
        privateData = other.privateData;

        return (*this);
    }

    Image Image::subImage(unsigned int x, unsigned int y, Size s) const {
        Image sub;
        // Check bounds
        if (!valid() ||
            x >= width() ||
            y >= height()) {
            return sub;
        }
        // Clip size to edges of image
        if (x + s.width > width()) {
            s.width = width() - x;
        }
        if (y + s.height > height()) {
            s.height = height() - y;
        }

        sub = Image(s, type(), Image::Discard, bytesPerRow());

        unsigned int offset = x*bytesPerPixel()+y*bytesPerRow();
        sub.setBuffer(buffer, data+offset);
        sub.bytesAllocated = bytesAllocated;
        sub.refCount = refCount;
        sub.mutex = mutex;
        sub.memMapped = memMapped;

        if (refCount) (*refCount)++;
        
        return sub;
    }

    Image Image::copy() const {
        Image duplicate;
        if (!valid()) {
            // Data is either Discard or AutoAllocate
            duplicate = Image(size(), type(), data);
        } else {
            // Got real data, allocate an image
            duplicate = Image(size(), type());
            duplicate.copyFrom(*this);
        }

        return duplicate;
    }

    void Image::copyFrom(const Image &srcImage) {
        if (!valid()) return;
        int h = std::min(srcImage.allocateHeight(), 
                         allocateHeight());
        int widthBytes = 
            std::min(srcImage.width()*srcImage.bytesPerPixel(), 
                     width()*bytesPerPixel());

        unsigned char *src = srcImage.data;
        unsigned char *dst = data;

        dprintf(DBG_MAJOR, "copyFrom: 0x%x to 0x%x pitch %d %d\n", src, dst, 
                srcImage.bytesPerRow(), bytesPerRow());
        for (int y=0; y < h; y++) {
            memcpy(dst, src, widthBytes);
            dst += bytesPerRow();
            src += srcImage.bytesPerRow();
        }
    }

    void Image::setBuffer(unsigned char *b, unsigned char *d) {
        if (holdingLock) pthread_mutex_unlock(mutex);
        holdingLock = false;

        if (refCount) {
            (*refCount)--;

            if (mutex && (*refCount == 0 || (weak() && *refCount == 1))) {
                pthread_mutex_destroy(mutex);
                delete mutex;
                mutex = NULL;
            }

            if (*refCount == 0) {
                delete refCount;
                if (memMapped) {
                    int success = munmap(buffer, bytesAllocated);
                    if (success == -1) {
                        error(Event::InternalError, 
                              "Image::setBuffer: Unable to unmap memory mapped region starting at %x of size %d: %s", 
                              buffer, bytesAllocated, strerror(errno));
                    }
                } else {
                    delete[] buffer;
                }
            }
            refCount = NULL;
            mutex = NULL;
        }

        if (b == Image::Discard ||
            b == Image::AutoAllocate) {
            buffer = NULL;
        } else {
            buffer = b;
        } 
        if (d == NULL) d = b;

        // This is the only place we're allowed to set the data field
        data = d;
    }
    
    bool Image::weak() {
        return (buffer == NULL);
    }
    
    bool Image::lock(int timeout) {
        if (holdingLock) {
            error(Event::ImageLockError, "Image reference trying to acquire lock it's already "
                  "holding. Make a separate image reference per thread.\n");
        } else if (!mutex) {
            error(Event::InternalError, "Locking an image with no mutex\n");
            holdingLock = false;
        } else if (timeout < 0) {
            pthread_mutex_lock(mutex);
            holdingLock = true;
        } else if (timeout == 0) {
            int ret = pthread_mutex_trylock(mutex);
            holdingLock = (ret == 0);
        } else {
            struct timespec t = (struct timespec)(Time::now() + timeout);
//! \todo fix the timedlock issue
#if defined(FCAM_ARCH_X86)
            int ret = pthread_mutex_trylock(mutex); // Temporary hack to compile on Cygwin, breaks semantics
#elif defined(FCAM_PLATFORM_ANDROID)
            // TODO: fix this - the pthread Android implementation doesn't have
            // pthread_mutex_timedlock
            int ret = pthread_mutex_trylock(mutex);
#else
            int ret = pthread_mutex_timedlock(mutex, &t);
#endif
            holdingLock = (ret == 0);
        }

        return holdingLock;
    }

    void Image::unlock() {
        if (!holdingLock) {
            error(Event::ImageLockError, "Cannot unlock a lock not held by this image reference");
            return;
        }
        if (!mutex) {
            error(Event::InternalError, "Unlocking an image with no mutex");
            debug();
            return;
        }
        pthread_mutex_unlock(mutex);
        holdingLock = false;
    }

    bool Image::operator==(const Image &other) const {
        if (data != other.data) return false;
        if (width() != other.width()) return false;
        if (height() != other.height()) return false;
        if (type() != other.type()) return false;
        /* bytesPerPixel, bytesPerRow, and buffer must match if the above do */
        return true;
    }

    void Image::debug(const char *name) const {
        printf("\tImage %s at %llx with dimensions %d %d type %d\n\t  bytes per pixel %d bytes per row %d\n\t  data %llx buffer %llx\n\t  refCount %llx = (%d), mutex %llx, memmapped %s, holdingLock %s\n",
               name,
               (long long unsigned)this,
               width(), height(),
               type(),
               bytesPerPixel(),
               bytesPerRow(),
               (long long unsigned)data,
               (long long unsigned)buffer,
               (long long unsigned)refCount,
               refCount ? *refCount : 0,
               (long long unsigned)mutex,
               (memMapped ? "true" : "false"),
               (holdingLock ? "true" : "false"));
    }

    unsigned int Image::allocateHeight() const {
        switch(_type) {
            case YUV420p:
                return height() + height()/2;
            default:
                return height();
        };
    }
   
}




