#include <sstream>
#include <sys/mman.h>
#include "string.h"

#include "FCam/processing/TIFF.h" 
#include <FCam/processing/Demosaic.h>
#include <FCam/Tegra/YUV420.h>
#include "TIFF.h"
#include "../Debug.h"

namespace FCam {

    void saveTIFF(Frame frame, std::string filename) 
    {
        Image im = frame.image();
        
        switch (im.type()) {
        case RAW:
            im = demosaic(frame);
            if (!im.valid()) {
                error(Event::FileSaveError, frame, "saveTIFF: %s: Cannot demosaic RAW image to save as TIFF.", filename.c_str());
                return;
            }
            // fall through to rgb24
        case RGB24: 
            saveTIFF(im, filename);
            break;
        case YUV420p:
            // Need to convert to RGB24 before saving
            im = Image(im.size(), RGB24);
            Tegra::convertYUV420ToRGB24(im, frame.image());
            break;
        case YUV24: case UYVY: 
        default:
            error(Event::FileSaveError, frame, "saveTIFF: %s: Unsupported image format", filename.c_str());
            break;
        }


        saveTIFF(im, filename);
    }

    void saveTIFF(Image im, std::string filename)
    {
        TiffFile tiff;
        TiffIfd *ifd0 = tiff.addIfd();
        ifd0->setImage(im);
        tiff.writeTo(filename);
    }

//
// Methods for TiffIfdEntry
//

    TiffIfdEntry::TiffIfdEntry(const RawTiffIfdEntry &entry, TiffFile *parent):
        entry(entry), info(NULL), parent(parent), state(UNREAD), val()
    {
        info = tiffEntryLookup(entry.tag);
    }

    TiffIfdEntry::TiffIfdEntry(uint16_t tag, const TagValue &val, TiffFile *parent):
        entry(), info(NULL), parent(parent), state(INVALID), val(val)
    {
        entry.tag = tag;
        entry.count = 0;
        entry.offset = 0;
        info = tiffEntryLookup(tag);
        if (info) entry.type = info->type;
        setValue(val);
    }

    TiffIfdEntry::TiffIfdEntry(uint16_t tag, TiffFile *parent): entry(), info(NULL), parent(parent), state(INVALID), val() {
        entry.tag = tag;
    }

    bool TiffIfdEntry::valid() const {
        return state != INVALID;
    }

    uint16_t TiffIfdEntry::tag() const {
        return entry.tag;
    }

    const char* TiffIfdEntry::name() const {
        if (info == NULL) return "UnknownTag";
        else return info->name;
    }

    const TagValue& TiffIfdEntry::value() const {
        if (state == UNREAD) {
            val = parse();
            if (!val.valid()) state = INVALID;
        }
        return val;
    }

    bool TiffIfdEntry::setValue(const TagValue &newVal) {
        if (info == NULL) {
            switch(newVal.type) {
            case TagValue::Null:
                warning(Event::FileSaveWarning, "TiffIfdEntry: NULL value passed in for tag %d", tag() );
                state = INVALID;
                return false;
                break;
            case TagValue::Int:
            case TagValue::IntVector:
                entry.type = TIFF_SLONG;
                break;
            case TagValue::Float:
            case TagValue::FloatVector:
                entry.type = TIFF_FLOAT;
                break;
            case TagValue::Double:
            case TagValue::DoubleVector:
                entry.type = TIFF_DOUBLE;
                break;
            case TagValue::String:
            case TagValue::StringVector:
                entry.type = TIFF_ASCII;
                break;
            case TagValue::Time:
            case TagValue::TimeVector:
                warning(Event::FileSaveWarning, "TiffIfdEntry: Can't store TagValue::Time values in TIFF tag %d", tag() );
                state = INVALID;
                val = TagValue();
                return false;
                break;
            }
        } else {
            bool typeMismatch = false;
            switch(newVal.type) {
            case TagValue::Null:
                warning(Event::FileSaveWarning, "TiffIfdEntry: NULL value passed in for tag %d", tag() );
                state = INVALID;
                return false;
                break;
            case TagValue::Int:
            case TagValue::IntVector:
                if (entry.type != TIFF_BYTE &&
                    entry.type != TIFF_SHORT &&
                    entry.type != TIFF_LONG &&
                    entry.type != TIFF_SBYTE &&
                    entry.type != TIFF_SSHORT &&
                    entry.type != TIFF_SLONG &&
                    entry.type != TIFF_IFD) {
                    typeMismatch= true;
                }
                break;
            case TagValue::Float:
            case TagValue::FloatVector:
                if (entry.type != TIFF_FLOAT &&
                    entry.type != TIFF_DOUBLE) {
                    typeMismatch= true;
                }
                break;
            case TagValue::Double:
            case TagValue::DoubleVector:
                if (entry.type != TIFF_DOUBLE &&
                    entry.type != TIFF_FLOAT &&
                    entry.type != TIFF_RATIONAL &&
                    entry.type != TIFF_SRATIONAL) {
                    typeMismatch= true;
                }
                break;
            case TagValue::String:
            case TagValue::StringVector:
                if (entry.type != TIFF_ASCII &&
                    entry.type != TIFF_BYTE &&
                    entry.type != TIFF_SBYTE) {
                    typeMismatch= true;
                }
                break;
            case TagValue::Time:
            case TagValue::TimeVector:
                typeMismatch = true;
                break;
            }
            if (typeMismatch) {
                warning(Event::FileSaveWarning, "TiffIfdEntry: Trying to set tag %d (%s) of type %d to incompatible TagValue type %d",
                        tag(), name(), entry.type, newVal.type);
                state = INVALID;
                return false;
            }
        }
        val = newVal;
        state = WRITTEN;
        return true;
    }

    bool TiffIfdEntry::writeDataBlock(FILE *fw) {
        const TagValue &v = value();
        if (state == INVALID) {
            error(Event::FileSaveError,
                  "TiffIfdEntry::writeDataBlock: Trying to write invalid tag %d (%s)",
                  tag(), name());
            return false;
        }

        unsigned int bytesPerElement=0;
        switch (entry.type) {
        case TIFF_BYTE:      bytesPerElement = 1; break;
        case TIFF_ASCII:     bytesPerElement = 1; break;
        case TIFF_SHORT:     bytesPerElement = 2; break;
        case TIFF_LONG:      bytesPerElement = 4; break;
        case TIFF_RATIONAL:  bytesPerElement = 8; break;
        case TIFF_SBYTE:     bytesPerElement = 1; break;
        case TIFF_UNDEFINED: bytesPerElement = 1; break;
        case TIFF_SSHORT:    bytesPerElement = 2; break;
        case TIFF_SLONG:     bytesPerElement = 4; break;
        case TIFF_SRATIONAL: bytesPerElement = 8; break;
        case TIFF_FLOAT:     bytesPerElement = 4; break;
        case TIFF_DOUBLE:    bytesPerElement = 8; break;
        case TIFF_IFD:       bytesPerElement = 4; break;
        }
        unsigned int elements = 0;
        switch (v.type) {
        case TagValue::Int:
        case TagValue::Float:
        case TagValue::Double:
            elements = 1;
            break;
        case TagValue::IntVector: {
            std::vector<int> &vi = v;
            elements = vi.size();
            break;
        }
        case TagValue::FloatVector: {
            std::vector<float> &vf = v;
            elements = vf.size();
            break;
        }
        case TagValue::DoubleVector: {
            std::vector<double> &vd = v;
            elements = vd.size();
            break;
        }
        case TagValue::String: {
            std::string &vs = v;
            if (entry.type == TIFF_ASCII) {
                elements = vs.size() + 1; // Account for having to add a null termination
            } else {
                elements = vs.size();
            }
            break;
        }
        case TagValue::StringVector: {
            std::vector<std::string> &strs = v;
            for (size_t i=0; i < strs.size(); i++) {
                elements += strs[i].size()+1; // Account for having to add a null termination
            }
            break;
        }
        default:
            error(Event::FileSaveError, "TiffIfdEntry::writeDataBlock: Unexpected TagValue type.");
            return false;
            break;
        }

        entry.count = elements;

        uint32_t dataBytes = bytesPerElement * elements;

        if (dataBytes <= 4) {
            // Just keep the data in the offset field
            switch(entry.type) {
            case TIFF_BYTE: {
                uint8_t *off = (uint8_t *)&entry.offset;
                if (v.type == TagValue::Int) {
                    uint8_t byte = (int)v;
                    *off = byte;
                } else if (v.type == TagValue::IntVector) {
                    std::vector<int> &bytes = v;
                    for (size_t i=0; i < elements; i++) {
                        *off++ = bytes[i];
                    }
                } else { // must be std::string
                    std::string &bytes = v;
                    for (size_t i=0; i < elements; i++) {
                        *off++ = bytes[i];
                    }
                }
                break;
            }
            case TIFF_ASCII: {
                uint8_t *off = (uint8_t *)&entry.offset;
                if (v.type == TagValue::String) {
                    std::string &ascii = v;
                    for (size_t i=0; i < ascii.size(); i++) {
                        *off++ = ascii[i];
                    }
                    *off = 0;
                } else { // must be std::vector<std::string>
                    std::vector<std::string> &asciis = v;
                    for (size_t i=0; i < asciis.size(); i++) {
                        for (size_t j=0; j < asciis[i].size(); j++) {
                            *off++ = asciis[i][j];
                        }
                        *off++ = 0;
                    }
                }
                break;
            }
            case TIFF_SHORT: {
                uint16_t *off = (uint16_t *)(void *)&entry.offset;
                if (v.type == TagValue::Int) {
                    uint16_t vs = (int)v;
                    *off = vs;
                } else { // must be int vector
                    std::vector<int> &shorts = v;
                    for (size_t i=0; i < elements; i++) {
                        *off++ = shorts[i];
                    }
                }
                break;
            }
            case TIFF_IFD:
            case TIFF_LONG: {
                if (v.type == TagValue::Int) {
                    uint32_t vs = (int)v;
                    entry.offset = vs;
                } else {
                    std::vector<int> &vi = v;
                    entry.offset = (uint32_t)vi[0];
                }
                break;
            }
            case TIFF_SBYTE:  {
                int8_t *off = (int8_t *)&entry.offset;
                if (v.type == TagValue::Int) {
                    int8_t byte = (int)v;
                    *off = byte;
                } else if (v.type == TagValue::IntVector) {
                    std::vector<int> &bytes = v;
                    for (size_t i=0; i < elements; i++) {
                        *off++ = bytes[i];
                    }
                } else { // must be std::string
                    std::string &bytes = v;
                    for (size_t i=0; i < elements; i++) {
                        *off++ = bytes[i];
                    }
                }
                break;
            }
            case TIFF_UNDEFINED: { // must be std::string
                uint8_t *off = (uint8_t *)&entry.offset;
                std::string &bytes = v;
                for (size_t i=0; i < elements; i++) {
                    *off++ = bytes[i];
                }
                break;
            }
            case TIFF_SSHORT: { // v must be int or std::vector<int>
                int16_t *off = (int16_t *)(void *)&entry.offset;
                if (v.type == TagValue::Int) {
                    int16_t vs = (int)v;
                    *off = vs;
                } else {
                    std::vector<int> &shorts = v;
                    for (size_t i=0; i < elements; i++) {
                        *off++ = shorts[i];
                    }
                }
                break;
            }
            case TIFF_SLONG: {
                if (v.type == TagValue::Int) {
                    int32_t vs = (int)v;
                    entry.offset = vs;
                } else { // must be int vector
                    std::vector<int> &vi = v;
                    entry.offset = vi[0];
                }
                break;
            }
            case TIFF_FLOAT: {
                float *off = (float *)(void *)&entry.offset;
                if (v.type == TagValue::Float) {
                    float vf = v;
                    *off = vf;
                } else { // must be float vector
                    std::vector<float> &vf = v;
                    *off = vf[0];
                }
                break;
            }
            }
        } else {
            // Entry data doesn't fit in the offset field
            // Need to write the data block to disk now
            
            // Without this guard, v.toString() probably always gets evaluated
            // if DEBUG is defined.
            #if FCAM_DEBUG_LEVEL >= 6
            dprintf(6, "TiffFileEntry::writeDataBlock: Tag %d (%s) data: %s\n", entry.tag, name(), v.toString().substr(0,200).c_str());
            #else
            dprintf(5, "TiffFileEntry::writeDataBlock: Writing tag %d (%s) data block.\n", entry.tag, name());
            #endif
            
            entry.offset = ftell(fw);
            // Data block must start on word boundary (even offset)
            if ( (entry.offset & 0x1) == 1) {
                uint8_t padding = 0x00;
                fwrite(&padding, sizeof(uint8_t), 1, fw);
                entry.offset = ftell(fw);
            }

            size_t written = 0;
            switch(entry.type) {
            case TIFF_BYTE: {
                if (v.type == TagValue::IntVector) {
                    std::vector<int> &vi = v;
                    std::vector<uint8_t> bytes(vi.begin(), vi.end());
                    written = fwrite(&bytes[0], sizeof(uint8_t), elements, fw);
                } else { // must be std::string
                    std::string &vs = v;
                    written = fwrite(vs.data(), sizeof(uint8_t), elements, fw);
                }
                break;
            }
            case TIFF_ASCII: {
                if (v.type == TagValue::String) {
                    std::string &ascii = v;
                    written = fwrite(ascii.c_str(), sizeof(char), elements, fw);
                } else { // must be std::vector<std::string>
                    std::vector<std::string> &asciis = v;
                    for (size_t i=0; i < asciis.size(); i++) {
                        written += fwrite(asciis[i].c_str(), sizeof(char), asciis[i].size()+1, fw);
                    }
                }
                break;
            }
            case TIFF_SHORT: { // v must be std::vector<int>
                std::vector<int> &vi = v;
                std::vector<uint16_t> shorts(vi.begin(), vi.end());
                written = fwrite(&shorts[0], sizeof(uint16_t), shorts.size(), fw);
                break;
            }
            case TIFF_IFD:
            case TIFF_LONG: { // v must be std::vector<int>
                std::vector<int> &vi = v;
                written = fwrite(&vi[0], sizeof(uint32_t), vi.size(), fw);
                break;
            }
            case TIFF_SRATIONAL:
            case TIFF_RATIONAL: {
                if (v.type == TagValue::Double) {
                    double vd = v;
                    if (entry.type == TIFF_RATIONAL && vd < 0 ) {
                        vd = 0;
                        warning(Event::FileSaveWarning, "TiffIfdEntry: Entry value less than zero when writing a RATIONAL entry. Clamped to zero.");
                    }
                    // \todo Fix Rationals
                    int32_t num = vd * (1 << 20);
                    int32_t den = 1 << 20;
                    written = fwrite(&num, sizeof(int32_t), 1, fw);
                    written += fwrite(&den, sizeof(int32_t), 1, fw);
                    written /= 2;
                } else {
                    std::vector<double> &vd = v;
                    written = 0;
                    for (size_t i=0; i < vd.size(); i++) {
                        if (entry.type == TIFF_RATIONAL && vd[i] < 0 ) {
                            vd[i] = 0;
                            warning(Event::FileSaveWarning, "TiffIfdEntry: Entry value less than zero when writing a RATIONAL entry. Clamped to zero.");
                        }
                        int32_t num = vd[i] * (1 << 20);
                        int32_t den = 1 << 20;
                        written += fwrite(&num, sizeof(int32_t), 1, fw);
                        written += fwrite(&den, sizeof(int32_t), 1, fw);
                    }
                    written /= 2;
                }
                break;
            }
            case TIFF_SBYTE:  {
                if (v.type == TagValue::IntVector) {
                    std::vector<int> &vi = v;
                    std::vector<int8_t> bytes(vi.begin(), vi.end());
                    written = fwrite(&bytes[0], sizeof(int8_t), elements, fw);
                } else { // must be std::string
                    std::string &vs = v;
                    written = fwrite(vs.data(), sizeof(int8_t), elements, fw);
                }
                break;
            }
            case TIFF_UNDEFINED: { // must be std::string
                std::string &vs = v;
                written = fwrite(vs.data(), sizeof(int8_t), elements, fw);
                break;
            }
            case TIFF_SSHORT: { // v must be std::vector<int>
                std::vector<int> &vi = v;
                std::vector<int16_t> shorts(vi.begin(), vi.end());
                written = fwrite(&shorts[0], sizeof(int16_t), elements, fw);
                break;
            }
            case TIFF_SLONG: { // v must be int vector
                std::vector<int> &vi = v;
                written = fwrite(&vi[0], sizeof(uint32_t), elements, fw);
                break;
            }
            case TIFF_FLOAT: { // v must be a float vector
                std::vector<float> &vf = v;
                written = fwrite(&vf[0], sizeof(float), elements, fw);
                break;
            }
            case TIFF_DOUBLE: {
                if (elements == 1) {
                    double vd = v;
                    written = fwrite(&vd, sizeof(double), elements, fw);
                } else {
                    std::vector<double> &vd = v;
                    written = fwrite(&vd[0], sizeof(double), elements, fw);
                }
                break;
            }
            }
            if (written != elements) {
                error(Event::FileSaveError, "TiffIfdEntry::writeDataBlock: Can't write data to file (tried to write %d, only wrote %d)", elements, written);
                return false;
            }
        }

        return true;
    }

    bool TiffIfdEntry::write(FILE *fw) {
        dprintf(5, "TIFFile::IfdEntry::write: Writing tag entry %d (%s): %d %d %d\n", tag(), name(), entry.type, entry.count, entry.offset);
        int count;

        // For compatibility with dcraw-derived applications, we never want to use TIFF type IFD, make them all LONGS instead
        uint16_t compatType = entry.type;
        if (compatType == TIFF_IFD) {
            compatType = TIFF_LONG;
        }

        count = fwrite(&entry.tag, sizeof(entry.tag), 1, fw);
        if (count == 1) fwrite(&compatType, sizeof(compatType), 1, fw);
        if (count == 1) fwrite(&entry.count, sizeof(entry.count), 1, fw);
        if (count == 1) fwrite(&entry.offset, sizeof(entry.offset), 1, fw);

        if (count != 1) {
            error(Event::FileSaveError, "TIFFile::IfdEntry::write: Can't write IFD entry to file.");
            return false;
        }
        return true;

    }

    bool TiffIfdEntry::operator<(const TiffIfdEntry &other) const {
        return tag() < other.tag();
    }

    TagValue TiffIfdEntry::parse() const {
        TagValue tag;

        if (info != NULL) {
            // Known tag, check for type mismatches
            bool typeMismatch=false;
            switch(info->type) {
            case TIFF_BYTE:
            case TIFF_SHORT:
            case TIFF_LONG:
                // Accept any unsigned integer type in any other's place
                if (entry.type != TIFF_BYTE &&
                    entry.type != TIFF_SHORT &&
                    entry.type != TIFF_LONG)
                    typeMismatch = true;
                break;
            case TIFF_SBYTE:
            case TIFF_SSHORT:
            case TIFF_SLONG:
                // Accept any signed integer type in any other's place
                if (entry.type != TIFF_SBYTE &&
                    entry.type != TIFF_SSHORT &&
                    entry.type != TIFF_SLONG)
                    typeMismatch = true;
                break;
            case TIFF_ASCII:
            case TIFF_RATIONAL:
            case TIFF_UNDEFINED:
            case TIFF_SRATIONAL:
            case TIFF_FLOAT:
            case TIFF_DOUBLE:
                // Strict matching for these types
                if (entry.type != info->type) typeMismatch = true;
                break;
            case TIFF_IFD:
                // Can also be LONG
                if (entry.type != TIFF_LONG &&
                    entry.type != TIFF_IFD) typeMismatch = true;
                break;
            }
            if (typeMismatch) {
                warning(Event::FileLoadWarning,
                        "In %s, type mismatch reading TIFF tag %d (%s), expected type %d, got %d\n",
                        parent->filename().c_str(), entry.tag, info->name, info->type, entry.type);
                return tag;
            }
        }
        // Now sort out how much data we're looking at
        unsigned int bytesPerElement=0;
        switch (entry.type) {
        case TIFF_BYTE:      bytesPerElement = 1; break;
        case TIFF_ASCII:     bytesPerElement = 1; break;
        case TIFF_SHORT:     bytesPerElement = 2; break;
        case TIFF_LONG:      bytesPerElement = 4; break;
        case TIFF_RATIONAL:  bytesPerElement = 8; break;
        case TIFF_SBYTE:     bytesPerElement = 1; break;
        case TIFF_UNDEFINED: bytesPerElement = 1; break;
        case TIFF_SSHORT:    bytesPerElement = 2; break;
        case TIFF_SLONG:     bytesPerElement = 4; break;
        case TIFF_SRATIONAL: bytesPerElement = 8; break;
        case TIFF_FLOAT:     bytesPerElement = 4; break;
        case TIFF_DOUBLE:    bytesPerElement = 8; break;
        case TIFF_IFD:       bytesPerElement = 4; break;
        }
        unsigned int totalBytes = entry.count*bytesPerElement;
        std::vector<uint8_t> data(totalBytes);

        // Read in the data, possibly seeking if needed
        if (entry.count > 4/bytesPerElement) {
            // Data doesn't fit in entry.offset field, go fetch it
            int adjOffset = parent->convLong(&entry.offset);
            bool success = parent->readByteArray(adjOffset, totalBytes,  &data[0]);
            if (!success) {
                warning(Event::FileLoadWarning,
                        "In %s, unable to read TIFF tag %d (%s) data at offset 0x%x\n",
                        parent->filename().c_str(), entry.tag, name(), entry.offset);
                return tag;
            }
        } else {
            uint8_t *ptr = (uint8_t*)&entry.offset;
            for (size_t i=0; i < data.size(); i++) {
                data[i] = *(ptr++);
            }
        }
        // Got undifferentiated byte soup, now interpret it and swap endianness as needed
        switch (entry.type) {
        case TIFF_BYTE:
        case TIFF_SBYTE:
        case TIFF_UNDEFINED:
            tag = std::string(data.begin(), data.end());
            break;
        case TIFF_ASCII: {
            // A TIFF ASCII field can contain multiple strings separated by null characters
            std::vector<std::string> strings;
            std::vector<uint8_t>::iterator start = data.begin();
            for (std::vector<uint8_t>::iterator it=data.begin(); it < data.end(); it++) {
                if (*it == 0) {
                    strings.push_back(std::string(start, it));
                    start = it + 1;
                }
            }
            if (strings.size() > 1) {
                tag = strings;
            } else {
                tag = strings[0];
            }
            break;
        }
        case TIFF_SHORT:
            if (entry.count > 1) {
                std::vector<int> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    vals[i] = parent->convShort(ptr);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                tag = parent->convShort(&data[0]);
            }
            break;
        case TIFF_IFD:
        case TIFF_LONG:
            // TagValues are signed, so possible reinterpretation problem here.
            if (entry.count > 1) {
                std::vector<int> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    vals[i] = (int)parent->convLong(ptr);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                tag = (int)parent->convLong(&data[0]);
            }
            break;
        case TIFF_RATIONAL:
            if (entry.count > 1) {
                std::vector<double> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    TiffRational r = parent->convRational(ptr);
                    // Precision loss here
                    vals[i] = ((double)r.numerator)/((double)r.denominator);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                TiffRational r = parent->convRational(&data[0]);
                tag = ((double)r.numerator)/((double)r.denominator);
            }
            break;
        case TIFF_SSHORT:
            if (entry.count > 1) {
                std::vector<int> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    uint16_t val = parent->convShort(ptr);
                    vals[i] = *reinterpret_cast<int16_t *>(&val);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                uint16_t val = parent->convShort(&data[0]);
                tag = *reinterpret_cast<int16_t *>(&val);
            }
            break;

        case TIFF_SLONG:
            if (entry.count > 1) {
                std::vector<int> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    uint32_t val = parent->convLong(ptr);
                    vals[i] = *reinterpret_cast<int32_t *>(&val);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                uint32_t val = parent->convLong(&data[0]);
                tag = *reinterpret_cast<int32_t *>(&val);
            }
            break;
        case TIFF_SRATIONAL:
            if (entry.count > 1) {
                std::vector<double> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    TiffRational r = parent->convRational(ptr);
                    // Precision loss here
                    vals[i] = (static_cast<double>(*reinterpret_cast<int32_t *>(&r.numerator)))
                        /(static_cast<double>(*reinterpret_cast<int32_t *>(&r.denominator)));
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                TiffRational r = parent->convRational(&data[0]);
                tag = (static_cast<double>(*reinterpret_cast<int32_t *>(&r.numerator)))
                        /(static_cast<double>(*reinterpret_cast<int32_t *>(&r.denominator)));
            }
            break;
        case TIFF_FLOAT:
            if (entry.count > 1) {
                std::vector<float> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    vals[i] = parent->convFloat(ptr);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                tag = parent->convFloat(&data[0]);
            }
            break;
        case TIFF_DOUBLE:
            if (entry.count > 1) {
                std::vector<double> vals(entry.count);
                uint8_t *ptr = &data[0];
                for (size_t i=0; i < entry.count; i++) {
                    vals[i] = parent->convDouble(ptr);
                    ptr+=bytesPerElement;
                }
                tag = vals;
            } else {
                tag = parent->convDouble(&data[0]);
            }
            break;
        };

        state = READ;

        return tag;
    }

//
// Methods for TiffIfd
//

    TiffIfd::TiffIfd(TiffFile *parent): parent(parent), exifIfd(NULL), imgState(UNREAD)  {
    }

    TiffIfd::~TiffIfd() {
        eraseSubIfds();
        eraseExifIfd();
    }

    const TiffIfdEntry* TiffIfd::find(uint16_t tag) const {
        entryMap::const_iterator match;
        match=entries.find(tag);
        if (match == entries.end()) return NULL;
        else return &(match->second);
    }

    TiffIfdEntry* TiffIfd::find(uint16_t tag) {
        entryMap::iterator match;
        match=entries.find(tag);
        if (match == entries.end()) return NULL;
        else return &(match->second);
    }

    bool TiffIfd::add(const RawTiffIfdEntry &rawEntry) {
        TiffIfdEntry entry(rawEntry, parent);

        entries.insert(entries.end(), entryMap::value_type(rawEntry.tag, entry));
        return true;
    }

    bool TiffIfd::add(uint16_t tag, const TagValue &val) {
        TiffIfdEntry entry(tag, val, parent);
        if (!entry.valid()) return false;
        TiffIfdEntry *existingEntry = find(tag);
        if (existingEntry) existingEntry->setValue(val);
        else {
            entries.insert(entryMap::value_type(tag, entry));
        }
        return true;
    }

    bool TiffIfd::add(const std::string &tagName, const TagValue &val) {
        const TiffEntryInfo *info = tiffEntryLookup(tagName);
        if (info) {
            return add(info->tag, val);
        }
        return false;
    }

    TiffIfd* TiffIfd::addSubIfd() {
        TiffIfd *ifd = new TiffIfd(parent);
        _subIfds.push_back(ifd);
        return ifd;
    }

    void TiffIfd::eraseSubIfds() {
        for (size_t i=0; i < _subIfds.size(); i++) {
            delete _subIfds[i];
        }
        _subIfds.clear();
    }

    TiffIfd* TiffIfd::addExifIfd() {
        if (exifIfd == NULL) exifIfd = new TiffIfd(parent);
        return exifIfd;
    }

    void TiffIfd::eraseExifIfd() {
        if (exifIfd != NULL) delete exifIfd;
    }

    const std::vector<TiffIfd *>& TiffIfd::subIfds() {
        return _subIfds;
    }

    TiffIfd* TiffIfd::subIfds(int index) const {
        return _subIfds[index];
    }

    Image TiffIfd::getImage(bool memMap) {
        if (imgState == NONE || imgState == CACHED) return imgCache;

        const TiffIfdEntry *entry;

        const char *file = parent->filename().c_str();

        entry = find(TIFF_TAG_PhotometricInterpretation);
        if (!entry) {
            imgState = NONE;
            return imgCache;
        }
        int photometricInterpretation = entry->value();

#define fatalError(...) \
        do { \
            warning(Event::FileLoadError, __VA_ARGS__);        \
            imgState = NONE; \
            return imgCache; \
        } while(0);

        ImageFormat fmt = UNKNOWN;
        switch (photometricInterpretation) {
        case TIFF_PhotometricInterpretation_WhiteIsZero:
        case TIFF_PhotometricInterpretation_BlackIsZero:
        case TIFF_PhotometricInterpretation_PaletteRGB:
        case TIFF_PhotometricInterpretation_TransparencyMask:
        case TIFF_PhotometricInterpretation_CMYK:
        case TIFF_PhotometricInterpretation_CIELAB:
        case TIFF_PhotometricInterpretation_ICCLAB:
        case TIFF_PhotometricInterpretation_ITULAB:
        case TIFF_PhotometricInterpretation_YCbCr:
            // Deal with unsupported formats first
            fatalError("TiffIfd::getImage(): %s: Unsupported pixel format (PhotometricInterpretation) %d.",
                       file,
                       photometricInterpretation);
            break;
        case TIFF_PhotometricInterpretation_RGB:
            fmt = RGB24;
            break;
        case TIFF_PhotometricInterpretation_LinearRaw:
            fatalError("TiffIfd::getImage(): %s: Linear RAW is not supported.",
                       file);
            break;
        case TIFF_PhotometricInterpretation_CFA:
            fmt = RAW;
            break;

        }

        int compression = TIFF_Compression_DEFAULT;
        entry = find(TIFF_TAG_Compression);
        if (entry) compression = entry->value();

        switch (compression) {
        case TIFF_Compression_Uncompressed:
            // ok
            break;
        default:
            fatalError("TiffIfd::getImage(): %s: Unsupported compression type %d.",
                       file,
                       compression);
            break;
        }

        // Now assuming uncompressed RAW or RGB24
        int samplesPerPixel = TIFF_SamplesPerPixel_DEFAULT;
        entry = find(TIFF_TAG_SamplesPerPixel);
        if (entry) samplesPerPixel = entry->value();
        switch (fmt) {
        case RAW:
            if (samplesPerPixel != 1) {
                fatalError("TiffIfd::getImage(): %s: RAW images cannot have more than 1 sample per pixel.",
                           file);
            } 
            break;
        case RGB24:
            if (samplesPerPixel != 3) {
                fatalError("TiffIfd::getImage(): %s: RGB24 images must have 3 samples per pixel", file);
            }
            break;               
        default: // shouldn't be here
            fatalError("TiffIfd::getImage(): %s: Unexpected branch in the road", file);
            break;
        }

        entry = find(TIFF_TAG_BitsPerSample);
        if (!entry) fatalError("TiffIfd::getImage(): %s: No BitsPerSample entry found.", file);

        switch (fmt) {
        case RAW: {
            int bitsPerSample = entry->value();
            if (bitsPerSample != 16) fatalError("TiffIfd::getImage(): %s: Only 16-bpp RAW images supported.", file);
            break;
        }
        case RGB24: {
            std::vector<int> bitsPerSample = entry->value();
            if (bitsPerSample[0] != 8 ||
                bitsPerSample[1] != 8||
                bitsPerSample[2] != 8) fatalError("TiffIfd::getImage(): %s: Only 24-bpp RGB images supported.", file);
            break;
        }
        default: // shouldn't be here
            fatalError("TiffIfd::getImage(): %s: Unexpected branch in the road", file);
            break;
        }

        // Read in image dimensions
        entry = find(TIFF_TAG_ImageWidth);
        if (!entry) fatalError("TiffIfd::getImage(): %s: No ImageWidth entry found.", file);
        int imageWidth = entry->value();

        entry = find(TIFF_TAG_ImageLength);
        if (!entry) fatalError("TiffIfd::getImage(): %s: No ImageLength entry found.", file);
        int imageLength = entry->value();

        dprintf(4,"TiffIfd::getImage(): %s: Image size is %d x %d\n", file, imageWidth, imageLength);


        // Read in image strip information
        entry = find(TIFF_TAG_RowsPerStrip);
        int rowsPerStrip = TIFF_RowsPerStrip_DEFAULT;
        uint32_t stripsPerImage = 1;
        if (entry) {
            rowsPerStrip = entry->value();
            stripsPerImage = imageLength / rowsPerStrip; // Full strips
            if (imageLength % rowsPerStrip != 0) stripsPerImage++; // Final partial strip
        }
        
        entry = find(TIFF_TAG_StripOffsets);
        if (!entry) fatalError("TiffIfd::getImage(): %s: No image strip data found, and tiled data is not supported.", file);
        std::vector<int> stripOffsets = entry->value();
        if (stripOffsets.size() != stripsPerImage)
            fatalError("TiffIfd::getImage(): %s: Malformed IFD - conflicting values on number of image strips.", file);
        
        dprintf(5, "TiffIfd::getImage(): %s: Image data in %d strips of %d rows each.\n", file, stripsPerImage, rowsPerStrip);
        
        uint32_t bytesPerStrip = rowsPerStrip * imageWidth * bytesPerPixel(fmt);
        uint32_t bytesLeft = imageLength * imageWidth * bytesPerPixel(fmt);

        // If memmapping requested, first confirm image data is contiguous
        if (memMap) {
            for (uint32_t strip=0; strip < stripsPerImage-1; strip++) {
                if (stripOffsets[strip]+(int)bytesPerStrip != stripOffsets[strip+1]) {
                    memMap = false;
                    warning(Event::FileLoadError, "TiffIfd::getImage(): %s: Memory mapped I/O was requested but TIFF image data is not contiguous.", file);
                    break;
                }
            }
        }

        if (!memMap) {
            //
            // Read in image data - standard I/O (non-cached)
            Image img(imageWidth, imageLength, fmt);
                        
            for (uint32_t strip=0; strip < stripsPerImage; strip++) {
                uint32_t bytesToRead = std::min(bytesLeft, bytesPerStrip);
                bool success = parent->readByteArray(stripOffsets[strip], 
                                                     bytesToRead, 
                                                     img(0,rowsPerStrip*strip) );
                if (!success) {
                    fatalError("TiffIfd::getImage(): %s: Cannot read in all image data.\n", file);
                }
                bytesLeft -= bytesToRead;
            }

            imgCache = img;
            imgState = CACHED;
        } else {
            // Read in image data - Memory mapped IO
            dprintf(5, "TiffIfd::getImage(): %s: Memmapping Image at %x, %x bytes\n",
                    file, stripOffsets[0], bytesPerPixel(fmt)*imageWidth*imageLength);
            imgCache = Image(fileno(parent->fp), 
                             stripOffsets[0], 
                             Size(imageWidth, imageLength), 
                             fmt);
        }
#undef fatalError

        return imgCache;
    }

    bool TiffIfd::setImage(Image newImg) {
        if (newImg.type() != RAW &&
            newImg.type() != RGB24) {
            error(Event::FileSaveError, "TiffIfd::setImage(): Can only save RAW or RGB24 images");
            return false;
        }
        imgCache = newImg;
        imgState = CACHED;
        return true;
    }

    bool TiffIfd::write(FILE *fw, uint32_t nextIfdOffset, uint32_t *offset) {
        bool success;
        // First write out all subIFDs, if any
        if (_subIfds.size() > 0) {
            std::vector<int> subIfdOffsets;
            // Write all subIfd data first
            for (size_t i=0; i < _subIfds.size(); i++ ) {
                uint32_t subIfdOffset;
                dprintf(4, "TiffIfd::write: Writing subIFD %d\n", i);
                success = _subIfds[i]->write(fw, 0, &subIfdOffset);
                if (!success) return false;
                subIfdOffsets.push_back(subIfdOffset);
            }
            // Then update our subIfd offset entry with returned locations
            TiffIfdEntry *subIfdEntry = find(TIFF_TAG_SubIFDs);
            if (subIfdEntry != NULL) {
                success = subIfdEntry->setValue(TagValue(subIfdOffsets));
            } else {
                success = add(TIFF_TAG_SubIFDs, TagValue(subIfdOffsets));
            }
            if (!success) return false;
        }
        // Then write out the EXIF ifd, if it exists
        if (exifIfd != NULL) {
            dprintf(4, "TiffIfd::write:  Writing exif IFD\n\n");
            uint32_t exifIfdOffset;
            success = exifIfd->write(fw, 0, &exifIfdOffset);
            if (!success) return false;
            success = add(EXIF_TAG_ExifIfd, (int)exifIfdOffset);
        }            
            
        // Then write out the image, if any
        success = writeImage(fw);
        if (!success) return false;

        // Then write out entry extra data fields
        dprintf(5, "TiffIfd::write: Writing Ifd entry data blocks.\n");

        for (entryMap::iterator it=entries.begin(); it != entries.end(); it++) {
            success = it->second.writeDataBlock(fw);
            if (!success) return false;
        }

        // Record starting offset for IFD
        *offset = ftell(fw);
        // IFD must start on word boundary (even byte offset)
        if ( (*offset & 0x1) == 1) {
            uint8_t padding = 0x00;
            fwrite(&padding, sizeof(uint8_t), 1, fw);
            *offset = ftell(fw);
        }

        // Now write out the IFD itself
        int count;
        uint16_t entryCount = entries.size();
        count = fwrite(&entryCount, sizeof(uint16_t), 1, fw);
        if (count != 1) return false;

        dprintf(5, "TiffIfd::write: Writing IFD entries\n");
        for (entryMap::iterator it=entries.begin(); it != entries.end(); it++) {
            success = it->second.write(fw);
            if (!success) return false;
        }
        count = fwrite(&nextIfdOffset, sizeof(uint32_t), 1, fw);
        if (count != 1) return false;

        dprintf(5, "TiffIfd::write: IFD written\n");
        return true;
    }

    bool TiffIfd::writeImage(FILE *fw) {
        Image img = getImage();
        if (imgState == NONE) return true;
        dprintf(5, "TiffIfd::writeImage: Beginning image write\n");
        if (!img.valid()) {
            error(Event::FileSaveError, "TiffIfd::writeImage: Invalid image");
            return false;
        }

        int photometricInterpretation = 0;
        int samplesPerPixel = 0;
        std::vector<int> bitsPerSample;
        switch (img.type()) {
        case RGB24:
            photometricInterpretation = TIFF_PhotometricInterpretation_RGB;
            samplesPerPixel = 3;
            bitsPerSample = std::vector<int>(3,8);
            break;
        case RGB16:
            photometricInterpretation = TIFF_PhotometricInterpretation_RGB;
            samplesPerPixel = 3;
            bitsPerSample.push_back(5);
            bitsPerSample.push_back(6);
            bitsPerSample.push_back(5);
            break;
        case UYVY:
        case YUV24:
        case YUV420p:
            error(Event::FileSaveError, "TiffIfd::writeImage: UYVY/YUV images not supported yet.\n");
            return false;
            break;
        case RAW:
            photometricInterpretation = TIFF_PhotometricInterpretation_CFA;
            samplesPerPixel = 1;
            bitsPerSample.push_back(16);
            break;
        case UNKNOWN:
            error(Event::FileSaveError,
                  "TiffIfd::writeImage: Can't save UNKNOWN images.");
            return false;
            break;
        }

        int width = img.width();
        int height = img.height();

        const uint32_t targetBytesPerStrip = 64 * 1024; // 64 K strips if possible
        const uint32_t minRowsPerStrip = 10; // But at least 10 rows per strip

        uint32_t bytesPerRow = img.bytesPerPixel() * width;
        int rowsPerStrip;
        if (minRowsPerStrip*bytesPerRow > targetBytesPerStrip) {
            rowsPerStrip = minRowsPerStrip;
        } else {
            rowsPerStrip = targetBytesPerStrip / bytesPerRow;
        }
        uint32_t stripsPerImage = height / rowsPerStrip;
        if (height % rowsPerStrip != 0) stripsPerImage++;

        std::vector<int> stripOffsets;
        std::vector<int> stripByteCounts;

        for (int ys=0; ys < height; ys += rowsPerStrip) {
            size_t lastRow = std::min(height, ys + rowsPerStrip);
            int bytesToWrite = (lastRow - ys) * bytesPerRow;

            stripOffsets.push_back(ftell(fw));
            stripByteCounts.push_back(bytesToWrite);

            int bytesWritten = 0;
            for (size_t y=ys; y < lastRow; y++) {
                bytesWritten += fwrite(img(0,y), sizeof(uint8_t), bytesPerRow, fw);
            }
            if (bytesWritten != bytesToWrite) {
                error(Event::FileSaveError, "TiffIfd::writeImage: Unable to write image data to file (wanted to write %d bytes, able to write %d).", bytesToWrite, bytesWritten);
                return false;
            }
        }

        bool success;
        success = add(TIFF_TAG_PhotometricInterpretation, photometricInterpretation);
        if (success) success = add(TIFF_TAG_SamplesPerPixel, samplesPerPixel);
        if (success) success = add(TIFF_TAG_BitsPerSample, bitsPerSample);

        if (success) success = add(TIFF_TAG_ImageWidth, width);
        if (success) success = add(TIFF_TAG_ImageLength, height);
        if (success) success = add(TIFF_TAG_RowsPerStrip, rowsPerStrip);

        if (success) success = add(TIFF_TAG_StripOffsets, stripOffsets);
        if (success) success = add(TIFF_TAG_StripByteCounts, stripByteCounts);
        // Not needed per spec, but dcraw seems to need this for thumbnails
        if (success) success = add(TIFF_TAG_Compression, TIFF_Compression_Uncompressed);

        if (!success) {
            error(Event::FileSaveError,
                  "TiffIfd::writeImage: Can't add needed tags to IFD");
            return false;
        }

        dprintf(5, "TiffIfd::writeImage: Image written.\n");
        return true;
    }
//
// Methods for TiffFile
//

    TiffFile::TiffFile(): valid(false), fp(NULL), offsetToIfd0(0) {
    }

    TiffFile::TiffFile(const std::string &file): valid(false), fp(NULL), offsetToIfd0(0) {
        readFrom(file);
    }

    TiffFile::~TiffFile() {
        eraseIfds();
        if (fp) fclose(fp);
    }

    bool TiffFile::readFrom(const std::string &file) {
        dprintf(DBG_MINOR, "TiffFile::readFrom(): Starting read of %s\n", file.c_str());

        // Make sure we start with a blank slate here
        if (fp) {
            fclose(fp); fp = NULL;
        }
        eraseIfds();

        valid = false;

        // Try to open the file
        fp = fopen(file.c_str(), "rb");
        _filename = file;
        if (fp == NULL) {
            std::stringstream errMsg;
            errMsg << "Unable to open file: "<<strerror(errno);
            setError("readFrom", errMsg.str());
            return false;
        }

        // Read in TIFF directories

        bool success = readHeader();
        if (!success) return false;

        uint32_t nextIfdOffset = offsetToIfd0;
        while (nextIfdOffset != 0) {
            TiffIfd *ifd = new TiffIfd(this);
            success = readIfd(nextIfdOffset, ifd, &nextIfdOffset);
            if (!success) {
                delete ifd;
                return false;
            }
            _ifds.push_back(ifd);
        }

        valid = true;
        return true;
    }

    bool TiffFile::writeTo(const std::string &file) {
        dprintf(4, "TIFFile::writeTo: %s: Beginning write\n", file.c_str());
        // Check that we have enough of an image to write
        if (ifds().size() == 0) {
            error(Event::FileSaveError,
                  "TiffFile::writeTo: %s: Nothing to write",
                  file.c_str());
            return false;
        }
        FILE *fw = NULL;
        fw = fopen(file.c_str(), "wb");
        if (!fw) {
            error(Event::FileSaveError,
                  "TiffFile::writeTo: %s: Can't open file for writing",
                  file.c_str());
            return false;
        }

        // Write out TIFF header
        int count = 0;
        uint32_t headerOffset = 0;
        count = fwrite(&littleEndianMarker, sizeof(littleEndianMarker), 1, fw);
        if (count == 1)
            count = fwrite(&tiffMagicNumber, sizeof(tiffMagicNumber), 1 , fw);
        // Write a dummy value for IFD0 offset for now. Will come back later, so store offset
        uint32_t headerIfd0Offset = ftell(fw);
        if (count == 1)
            count = fwrite(&headerOffset, sizeof(headerOffset), 1, fw);
        if (count != 1) {
            error(Event::FileSaveError,
                  "TiffFile::writeTo: %s: Can't write TIFF file header",
                  file.c_str());
            fclose(fw);
            return false;
        }

        // Write out all the IFDs, reverse order to minimize fseeks
        bool success;
        uint32_t nextIfdOffset = 0;
        for (size_t i=ifds().size(); i > 0; i--) {
            dprintf(4, "TIFFile::writeTo: %s: Writing IFD %d\n", file.c_str(), i-1);
            success = ifds(i-1)->write(fw, nextIfdOffset, &nextIfdOffset);
            if (!success) {
                error(Event::FileSaveError,
                      "TiffFile::writeTo: %s: Can't write entry data blocks",
                      file.c_str());
                fclose(fw);
                return false;
            }
        }

        // Go back to the start and write the offset to the first IFD (last written)
        fseek(fw, headerIfd0Offset, SEEK_SET);
        count = fwrite(&nextIfdOffset, sizeof(uint32_t), 1, fw);
        if (count != 1) {
            error(Event::FileSaveError,
                  "TiffFile::writeTo: %s: Can't write Ifd offset into header",
                  file.c_str());
            fclose(fw);
            return false;
        }

        fclose(fw);
        return true;
    }

    const std::string& TiffFile::filename() const {
        return _filename;
    }

    TiffIfd* TiffFile::addIfd() {
        TiffIfd *ifd = new TiffIfd(this);
        _ifds.push_back(ifd);
        return ifd;
    }

    void TiffFile::eraseIfds() {
        for (size_t i=0; i < _ifds.size(); i++) {
            delete _ifds[i];
        }
        _ifds.clear();
    }

    const std::vector<TiffIfd *>& TiffFile::ifds() const {
        return _ifds;
    }

    TiffIfd* TiffFile::ifds(int index) {
        return _ifds[index];
    }

// TIFF subpart-reading functions. These access the file pointer and seek around the TIFF file, so don't trust
// the file pointer to be in the right place after calling these.

    bool TiffFile::readHeader() {
        // Read in the TIFF header at start of file
        fseek(fp, 0, SEEK_SET);

        int count;
        uint16_t byteOrder, tiffHeaderNumber;
        count = fread(&byteOrder, sizeof(byteOrder), 1, fp);
        if (count == 1)
            count = fread(&tiffHeaderNumber, sizeof(tiffHeaderNumber), 1, fp);
        if (count == 1)
            count = fread(&offsetToIfd0, sizeof(offsetToIfd0), 1, fp);
        if (count != 1) {
            setError("readHeader", "Unable to read TIFF header!");
            fclose(fp); fp = NULL;
            return false;
        }

        // Then find out the endianness of the TIFF file.
        if (byteOrder == littleEndianMarker) {
            littleEndian = true;
        } else if (byteOrder == bigEndianMarker) {
            littleEndian = false;
        } else {
            setError("readHeader", "Malformed TIFF header");
            fclose(fp); fp = NULL;
            return false;
        }
        dprintf(4, "TiffFile::readHeader(): %s is %s-endian\n", filename().c_str(), littleEndian ? "little" : "big");

        // Now we can use the convXXX convenience functions, and check the magic number
        tiffHeaderNumber = convShort(&tiffHeaderNumber);
        if (tiffHeaderNumber != tiffMagicNumber) {
            std::stringstream errMsg;
            errMsg << "TIFF header magic number is incorrect. This is not a valid TIFF or DNG file. (got "
                   <<tiffHeaderNumber<<", expected "<<tiffMagicNumber;
            setError("readHeader", errMsg.str());
            fclose(fp); fp = NULL;
            return false;
        }

        // Let's get the offset to the start of the first directory
        offsetToIfd0 = convLong(&offsetToIfd0);
        dprintf(4, "TiffFile::readHeader(): %s: IFD0 offset is 0x%x\n", filename().c_str(), offsetToIfd0);

        // Sanity check it
        if (offsetToIfd0 < 8) {
            std::stringstream errMsg;
            errMsg << "Offset to first IFD in TIFF file is " << offsetToIfd0 
                   << ". This is not a valid TIFF or DNG file.";
            setError("readHeader", errMsg.str());
            fclose(fp); fp = NULL;
            return false;
        }

        return true;
    }

    bool TiffFile::readIfd(uint32_t offsetToIFD, TiffIfd *ifd, uint32_t *offsetToNextIFD) {
        int err;

        err = fseek(fp, offsetToIFD, SEEK_SET);
        if (err != 0) {
            std::stringstream errMsg;
            errMsg << "Unable to seek to IFD at "<<offsetToIFD << ": "<<strerror(errno);
            setError("readIfd", errMsg.str());
            fclose(fp); fp = NULL;
            return false;
        }

        // Read in number of entries in IFD
        int count;
        uint16_t ifdEntries;
        count = fread(&ifdEntries, sizeof(uint16_t), 1, fp);
        if (count != 1) {
            std::stringstream errMsg;
            errMsg << "Unable to read header for IFD at "<<offsetToIFD;
            setError("readIfd", errMsg.str());
            fclose(fp); fp = NULL;
            return false;
        }
        ifdEntries = convShort(&ifdEntries);
        dprintf(4, "TiffFile::readIfd(): In %s, IFD at 0x%x contains %d entries\n", filename().c_str(), (int)offsetToIFD, (int)ifdEntries);

        // Read in IFD entries
        for (int i=0; i < ifdEntries; i++) {
            RawTiffIfdEntry entry;
            count = fread(&entry.tag, sizeof(entry.tag), 1, fp);
            if (count == 1)
                count = fread(&entry.type, sizeof(entry.type), 1, fp);
            if (count == 1)
                count = fread(&entry.count, sizeof(entry.count), 1, fp);
            if (count == 1)
                count = fread(&entry.offset, sizeof(entry.offset), 1, fp);
            if (count != 1) {
                std::stringstream errMsg;
                errMsg << "Unable to read IFD entry "<<i <<" for IFD at "<<offsetToIFD;
                setError("readIfd", errMsg.str());
                fclose(fp); fp = NULL;
                return false;
            }
            entry.tag = convShort(&entry.tag);
            entry.type = convShort(&entry.type);
            entry.count = convLong(&entry.count);
            // Not endian-adjusting entry.offset, as its interpretation is not known yet
            dprintf(5, "TiffFile::readIfd(): IFD entry %d: Tag: %d (%s), Type: %d, Count: %d, Offset: 0x%x\n",
                    i, entry.tag, tiffEntryLookup(entry.tag) == NULL ? "Unknown" : tiffEntryLookup(entry.tag)->name, entry.type, entry.count, entry.offset);
            ifd->add(entry);
        }

        // Read in next IFD offset
        if (offsetToNextIFD) {
            count = fread(offsetToNextIFD, sizeof(uint32_t), 1, fp);
            if (count != 1) {
                std::stringstream errMsg;
                errMsg << "Unable to read next-IFD offset field for IFD at "<<offsetToIFD;
                setError("readIfd", errMsg.str());
                fclose(fp); fp = NULL;
                return false;
            }
            *offsetToNextIFD = convLong(offsetToNextIFD);
            dprintf(DBG_MINOR, "TiffFile::readIfd(): In file %s, IFD at %x has next-IFD offset field of %x\n",
                    filename().c_str(), offsetToIFD, *offsetToNextIFD);
        }

        bool success = readSubIfds(ifd);

        return success;
    }

    bool TiffFile::readSubIfds(TiffIfd *ifd) {
        // Read in subIFDs
        ifd->eraseSubIfds();

        bool success;
        const TiffIfdEntry *subIfdEntry = ifd->find(TIFF_TAG_SubIFDs);
        if (subIfdEntry) {
            TagValue val = subIfdEntry->value();
            if (!val.valid()) {
                setError("readSubIfds", "Unable to read TIFF subIFDs");
                fclose(fp); fp = NULL;
                return false;
            }
            std::vector<int> subIfdOffsets;
            if (val.type == TagValue::Int) {
                subIfdOffsets.push_back(val.asInt());
            } else {
                subIfdOffsets = val;
            }
            dprintf(DBG_MINOR, "TiffFile::readSubIfds(): %s: IFD has %d subIFDs\n",
                    filename().c_str(), subIfdOffsets.size());
            for (unsigned int i=0; i < subIfdOffsets.size(); i++) {
                TiffIfd *subIfd = ifd->addSubIfd();
                // Not supporting subIfd chains, just trees
                success = readIfd(subIfdOffsets[i], subIfd, NULL);
                if (!success) {
                    return false;
                }
            }
        }

        return true;
    }

    uint16_t TiffFile::convShort(void const *src) {
        uint16_t s = *(uint16_t const *)src;
        if (!littleEndian) s = (s << 8) || (s >> 8);
        return s;
    }

    uint32_t TiffFile::convLong(void const *src) {
        uint32_t l = *(uint32_t const *)src;
        if (!littleEndian) l = ((l & 0x000000FF) << 24) ||
                               ((l & 0x0000FF00) << 8 ) ||
                               ((l & 0x00FF0000) >> 8 ) ||
                               ((l & 0xFF000000) >> 24 );
        return l;
    }

    float TiffFile::convFloat(void const *src) {
        return *reinterpret_cast<float *>(convLong(src));
    }

    double TiffFile::convDouble(void const *src) {
        double d;
        if (!littleEndian) {
            uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
            for (int i=0;i<  8;i++) {
                *(ptr++) = *(((uint8_t const *)src)+7-i);
            }
        } else {
            d = *reinterpret_cast<double const *>(src);
        }
        return d;
    }

    bool TiffFile::readByteArray(uint32_t offset, uint32_t count, uint8_t *dest) {
        if (!dest) return false;

        int err = fseek(fp, offset, SEEK_SET);
        if (err != 0) {
            return false;
        }
        size_t trueCount = fread(dest, sizeof(uint8_t), count, fp);
        if (trueCount != count) return false;

        return true;
    }

    bool TiffFile::readShortArray(uint32_t offset, uint32_t count, uint16_t *dest) {
        if (!dest) return false;

        int err = fseek(fp, offset, SEEK_SET);
        if (err != 0) {
            return false;
        }
        size_t trueCount = fread(dest, sizeof(uint16_t), count, fp);
        if (trueCount != count) return false;

        if (!littleEndian) {
            for (size_t i=0; i < count; i++) {
                uint16_t s = dest[i];
                dest[i] = (s << 8) || (s >> 8);
            }
        }
        return true;
    }

    TiffRational TiffFile::convRational(void const *src) {
        TiffRational r;
        r.numerator = convLong(src);
        r.denominator = convLong((unsigned char *)src+4);
        return r;
    }


}
