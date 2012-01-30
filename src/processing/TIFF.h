#ifndef FCAM_TIFF_H
#define FCAM_TIFF_H

#include <string>
#include <map>
#include <vector>

#include <FCam/Image.h>
#include <FCam/TagValue.h>
#include <FCam/Event.h>

#include "TIFFTags.h"

namespace FCam {

    class TiffFile;
    class TiffIfd;
    class TiffIfdEntry;

    // A class representing a TIFF directory entry
    // Only reads in its data when asked for, which may require file IO
    class TiffIfdEntry {
    public:
        // Construct an IfdEntry from a raw TIFF IFD structure
        // Used when loading a TIFF file
        TiffIfdEntry(const RawTiffIfdEntry &entry, TiffFile *parent);
        // Construct an IfdEntry from a tag/value pair
        // Used when creating/modifying a TIFF file
        TiffIfdEntry(uint16_t tag, const TagValue &val, TiffFile *parent);
        // Construct an IfdEntry with just a tag
        // Used for searching for a matching IfdEntry in an Ifd
        TiffIfdEntry(uint16_t tag, TiffFile *parent=NULL);

        // Check for entry validity. May not be valid due to
        // type mismatches
        bool valid() const;

        // Retrieve the TIFF tag number of this entry
        uint16_t tag() const;
        // Retrieve the name of this entry
        const char *name() const;
        // Retrieve the current value of this entry
        // May cause file IO if loading a TIFF file and this
        // entry has not been read before
        const TagValue& value() const;

        // Change the value of this entry
        bool setValue(const TagValue &);
        // Writes excess data to file, and updates local offset pointer
        bool writeDataBlock(FILE *fw);
        // Writes entry to file. Assumes writeDataBlock has already been done to update entry offset field.
        bool write(FILE *fw);

        bool operator<(const TiffIfdEntry &other) const;
    private:
        RawTiffIfdEntry entry;
        const TiffEntryInfo *info;
        TiffFile *parent;

        TagValue parse() const;

        mutable enum {
            INVALID,
            UNREAD,
            READ,
            WRITTEN
        } state; // Tracks caching state

        mutable TagValue val; // Cached value
    };

    // An object representing a TIFF directory, with accessors for
    // interpreting/constructing them
    class TiffIfd {
    public:
        TiffIfd(TiffFile *parent);
        ~TiffIfd();

        // Return a pointer to the raw IFD entry structure matching tag in the given IFD, if one
        // exists, NULL otherwise
        const TiffIfdEntry *find(uint16_t tag) const;
        TiffIfdEntry *find(uint16_t tag);

        // Adds an entry created from a raw TIFF entry structure
        // Used in reading a TIFF.
        // Does not replace an existing entry if there is one already
        // with the same tag.
        bool add(const RawTiffIfdEntry &);
        // Adds an entry with a given tag and value.
        // In case of type mismatch, returns false.
        // Replaces the existing tag if there is one.
        // Used when writing a TIFF.
        bool add(uint16_t tag, const TagValue &val);
        // Adds an entry with a given tag name and value. If name
        // is unknown, returns false.  Used when writing a TIFF.
        // Replaces an existing tag if there is one.
        bool add(const std::string &tagName, const TagValue &val);

        // Constructs a new subIfd for this Ifd, and returns a pointer to it
        TiffIfd* addSubIfd();
        // Erase all subIfds
        void eraseSubIfds();

        // Adds an Exif IFD to the Ifd, or returns pointer to existing one
        TiffIfd* addExifIfd();
        // Removes existing Exif IFD
        void eraseExifIfd();

        // Get a reference to a vector of subIfds
        const std::vector<TiffIfd *> &subIfds();
        // Get a pointer to a specific subIfd
        TiffIfd* subIfds(int index) const;

        // Constructs the image referred to by this Ifd. Will require IO to the file if the image
        // hasn't been read already. Optionally, use memory mapped IO to manage the image memory.
	// This is only allowable for images that have been stored contiguously in the source file.
        Image getImage(bool memMap = true);
        // Sets the image to be saved in this Ifd.
        bool setImage(Image newImg);

        // Write all entries, subIFds, and image data to file
        // Retuns success/failure, and the starting location of the Ifd in
        // the file in offset.
        bool write(FILE *fw, uint32_t prevIfdOffset, uint32_t *offset);
    private:
        TiffFile * const parent;

        std::vector<TiffIfd *> _subIfds;
        TiffIfd *exifIfd;

        typedef std::map<int, TiffIfdEntry> entryMap;
        entryMap entries;

        enum {
            UNREAD,
            NONE,
            CACHED
        } imgState;
        Image imgCache;

        // Subfunction to write image data out, and to update the IFD
        // entry offsets for it
        bool writeImage(FILE *fw);
    };

    // High-level interface to reading and writing TIFF
    // files. Implemented functionality limited to those needed for
    // DNG file access (uncompressed striped data, only a few color
    // spaces)
    class TiffFile {
    public:

        TiffFile(const std::string &file);
        TiffFile();
        ~TiffFile();

        bool readFrom(const std::string &file);
        bool writeTo(const std::string &file);

        bool valid;
        const std::string &filename() const;

        // Add a new top-level IFD to the file, and return a pointer to it
        TiffIfd *addIfd();
        // Remove all Ifds from the file
        void eraseIfds();

        // Access the whole set of Ifds in this file
        const std::vector<TiffIfd *> &ifds() const;
        // Get a pointer to a specific Ifd
        TiffIfd* ifds(int index);

        Event lastEvent;
    private:
        FILE *fp;
        std::string _filename;

        bool littleEndian;
        uint32_t offsetToIfd0;

        std::vector<TiffIfd *> _ifds;

        uint16_t convShort(void const *src);
        uint32_t convLong(void const *src);
        float convFloat(void const *src);
        double convDouble(void const *src);
        TiffRational convRational(void const *src);

        // Read an array of bytes from the file.
        bool readByteArray(uint32_t offset, uint32_t count, uint8_t *data);

        // Read an array of shorts (probably image data) from the file into dest
        bool readShortArray(uint32_t offset, uint32_t count, uint16_t *dest);

        bool readHeader();
        bool readIfd(uint32_t offsetToIFD, TiffIfd *ifd, uint32_t *offsetToNextIFD=NULL);
        bool readSubIfds(TiffIfd *ifd);

        void setError(std::string module, std::string description) {
            lastEvent.creator = NULL;
            lastEvent.type = Event::Error;
            lastEvent.data = Event::FileLoadError;
            lastEvent.time = Time::now();
            lastEvent.description = "TiffFile::"+module+":"+filename()+": "+description;
        }

        friend class TiffIfd;
        friend class TiffIfdEntry;
    };


}

#endif
