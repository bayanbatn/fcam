#ifndef FCAM_TIFFTAGS_H
#define FCAM_TIFFTAGS_H

#include "string.h"
#include <string>
#include <vector>
#include <stdint.h>

namespace FCam {

    // TIFF basic structures
    enum TiffType {
        TIFF_BYTE = 1,
        TIFF_ASCII = 2,
        TIFF_SHORT = 3,
        TIFF_LONG = 4,
        TIFF_RATIONAL = 5,
        TIFF_SBYTE = 6,
        TIFF_UNDEFINED = 7,
        TIFF_SSHORT = 8,
        TIFF_SLONG = 9,
        TIFF_SRATIONAL = 10,
        TIFF_FLOAT = 11,
        TIFF_DOUBLE=12,
        TIFF_IFD=13
    };

    struct TiffEntryInfo {
        const char *name;
        uint16_t tag;
        TiffType type;
    };

    struct TiffRational {
        uint32_t numerator;
        uint32_t denominator;
    };

    struct RawTiffIfdEntry {
        uint16_t tag;
        uint16_t type;
        uint32_t count;
        uint32_t offset;
    };

    // TIFF header constants
    const uint32_t headerBytes = 8;
    const uint16_t littleEndianMarker = 0x4949;
    const uint16_t bigEndianMarker = 0x4D4D;
    const uint16_t tiffMagicNumber = 42;

    // TIFF IFD entry information table and fast lookup functions
    extern const TiffEntryInfo tiffEntryTypes[];
    const TiffEntryInfo *tiffEntryLookup(uint16_t tag);
    const TiffEntryInfo *tiffEntryLookup(const std::string &entryName);

    // Hard-coded tag values for various TIFF tags, for faster parsing/filtering
    const uint16_t        TIFF_TAG_NewSubFileType                      = 254;
    const uint16_t        TIFF_TAG_ImageWidth                          = 256;
    const uint16_t        TIFF_TAG_ImageLength                         = 257;
    const uint16_t        TIFF_TAG_BitsPerSample                       = 258;
    const uint16_t        TIFF_TAG_Compression                         = 259;
    const uint16_t        TIFF_TAG_PhotometricInterpretation           = 262;
    const uint16_t        TIFF_TAG_Make                                = 271;
    const uint16_t        TIFF_TAG_Model                               = 272;
    const uint16_t        TIFF_TAG_StripOffsets                        = 273;
    const uint16_t        TIFF_TAG_Orientation                         = 274;
    const uint16_t        TIFF_TAG_SamplesPerPixel                     = 277;
    const uint16_t        TIFF_TAG_RowsPerStrip                        = 278;
    const uint16_t        TIFF_TAG_StripByteCounts                     = 279;
    const uint16_t        TIFF_TAG_PlanarConfiguration                 = 284;
    const uint16_t        TIFF_TAG_ResolutionUnit                      = 296;
    const uint16_t        TIFF_TAG_Software                            = 305;
    const uint16_t        TIFF_TAG_DateTime                            = 306;

    const uint16_t        TIFFEP_TAG_CFARepeatPatternDim               = 33421;
    const uint16_t        TIFFEP_TAG_CFAPattern                        = 33422;
    const uint16_t        TIFFEP_TAG_Flash                             = 37385;
    const uint16_t        TIFFEP_TAG_TIFFEPStandardID                  = 37398;

    // tags from first TIFF supplement
    const uint16_t        TIFF_TAG_SubIFDs                             = 330;

    // DNG tags
    const uint16_t        DNG_TAG_DNGVersion                           = 50706;
    const uint16_t        DNG_TAG_DNGBackwardVersion                   = 50707;
    const uint16_t        DNG_TAG_UniqueCameraModel                    = 50708;
    const uint16_t        DNG_TAG_LocalizedCameraModel                 = 50709;
    const uint16_t        DNG_TAG_CFAPlaneColor                        = 50710;
    const uint16_t        DNG_TAG_CFALayout                            = 50711;
    const uint16_t        DNG_TAG_BlackLevelRepeatDim                  = 50713;
    const uint16_t        DNG_TAG_BlackLevel                           = 50714;
    const uint16_t        DNG_TAG_BlackLevelDeltaH                     = 50715;
    const uint16_t        DNG_TAG_BlackLevelDeltaV                     = 50716;
    const uint16_t        DNG_TAG_WhiteLevel                           = 50717;
    const uint16_t        DNG_TAG_DefaultScale                         = 50718;
    const uint16_t        DNG_TAG_DefaultCropOrigin                    = 50719;
    const uint16_t        DNG_TAG_DefaultCropSize                      = 50720;
    const uint16_t        DNG_TAG_ColorMatrix1                         = 50721;
    const uint16_t        DNG_TAG_ColorMatrix2                         = 50722;
    const uint16_t        DNG_TAG_CameraCalibration1                   = 50723;
    const uint16_t        DNG_TAG_CameraCalibration2                   = 50724;
    const uint16_t        DNG_TAG_ReductionMatrix1                     = 50725;
    const uint16_t        DNG_TAG_ReductionMatrix2                     = 50726;
    const uint16_t        DNG_TAG_AnalogBalance                        = 50727;
    const uint16_t        DNG_TAG_AsShotNeutral                        = 50728;
    const uint16_t        DNG_TAG_AsShotWhiteXY                        = 50729;
    const uint16_t        DNG_TAG_BaselineExposure                     = 50730;
    const uint16_t        DNG_TAG_BaselineNoise                        = 50731;
    const uint16_t        DNG_TAG_BaselineSharpness                    = 50732;
    const uint16_t        DNG_TAG_BayerGreenSplit                      = 50733;
    const uint16_t        DNG_TAG_LinearResponseLimit                  = 50734;
    const uint16_t        DNG_TAG_LensInfo                             = 50736;
    const uint16_t        DNG_TAG_ChromaBlurRadius                     = 50737;
    const uint16_t        DNG_TAG_AntiAliasStrength                    = 50738;
    const uint16_t        DNG_TAG_ShadowScale                          = 50739;
    const uint16_t        DNG_TAG_DNGPrivateData                       = 50740;
    const uint16_t        DNG_TAG_MakerNoteSafety                      = 50741;
    const uint16_t        DNG_TAG_CalibrationIlluminant1               = 50778;
    const uint16_t        DNG_TAG_CalibrationIlluminant2               = 50779;
    const uint16_t        DNG_TAG_BestQualityScale                     = 50780;
    const uint16_t        DNG_TAG_ActiveArea                           = 50829;
    const uint16_t        DNG_TAG_MaskedAreas                          = 50830;

    // tags from version 1.3.0.0
    const uint16_t        DNG_TAG_NoiseProfile                         = 51041;

    // EXIF tags
    const uint16_t        EXIF_TAG_ExifIfd                             = 34665;
    const uint16_t        EXIF_TAG_ExposureTime                        = 33434;
    const uint16_t        EXIF_TAG_FNumber                             = 33437;
    const uint16_t        EXIF_TAG_FocalLength                         = 37386;
    const uint16_t        EXIF_TAG_SubsecTime                          = 37520;
    const uint16_t        EXIF_TAG_ISOSpeedRatings                     = 34855;
    const uint16_t        EXIF_TAG_SensitivityType                     = 34864;

    // Various values for the above fields

    const uint32_t TIFF_NewSubfileType_FullRAW = 0;
    const uint32_t TIFF_NewSubfileType_MainPreview = 1;
    const uint32_t TIFF_NewSubfileType_OtherPreview = 0x10001;
    const uint32_t TIFF_NewSubfileType_DEFAULT = 0;

    const uint16_t TIFF_PhotometricInterpretation_WhiteIsZero = 0;
    const uint16_t TIFF_PhotometricInterpretation_BlackIsZero = 1;
    const uint16_t TIFF_PhotometricInterpretation_RGB = 2;
    const uint16_t TIFF_PhotometricInterpretation_PaletteRGB = 3;
    const uint16_t TIFF_PhotometricInterpretation_TransparencyMask = 4;
    const uint16_t TIFF_PhotometricInterpretation_CMYK = 5;
    const uint16_t TIFF_PhotometricInterpretation_YCbCr = 6;
    const uint16_t TIFF_PhotometricInterpretation_CIELAB = 8;
    const uint16_t TIFF_PhotometricInterpretation_ICCLAB = 9;
    const uint16_t TIFF_PhotometricInterpretation_ITULAB = 10;
    const uint16_t TIFF_PhotometricInterpretation_CFA = 32803;
    const uint16_t TIFF_PhotometricInterpretation_LinearRaw = 34892;

    const uint16_t TIFF_Compression_Uncompressed = 1;
    const uint16_t TIFF_Compression_LZW = 5;
    const uint16_t TIFF_Compression_JPEG_old = 6;
    const uint16_t TIFF_Compression_JPEG = 7;
    const uint16_t TIFF_Compression_DEFAULT = TIFF_Compression_Uncompressed;

    // First term is what 0th row represents, the second is what 0th column represents
    const uint16_t TIFF_Orientation_TopLeft = 1;
    const uint16_t TIFF_Orientation_TopRight = 2;
    const uint16_t TIFF_Orientation_BottomLeft = 3;
    const uint16_t TIFF_Orientation_BottomRight = 4;
    const uint16_t TIFF_Orientation_LeftTop = 5;
    const uint16_t TIFF_Orientation_RightTop = 6;
    const uint16_t TIFF_Orientation_LeftBottom = 7;
    const uint16_t TIFF_Orientation_RightBottom = 8;

    const uint16_t TIFF_SamplesPerPixel_DEFAULT = 1;

    const uint32_t TIFF_RowsPerStrip_DEFAULT = 0xFFFFFFFF;

    const char TIFFEP_CFAPattern_RGGB[] = {00,01,01,02};
    const char TIFFEP_CFAPattern_BGGR[] = {02,01,01,00};
    const char TIFFEP_CFAPattern_GRBG[] = {01,00,02,01};
    const char TIFFEP_CFAPattern_GBRG[] = {01,02,00,01};

    // Table of calibration illuminant color temperatures
    const unsigned int DNG_CalibrationIlluminant_Values = 26;
    const unsigned int DNG_CalibrationIlluminant_Temp[] = {
        0, // 0 = Unknown
        // Values below from personal preference, less
        // known internet sources.
        6500, // 1 = Daylight
        5000, // 2 = Fluorescent
        3200, // 3 = Tungsten (incandescent light)
        5600, // 4 = Flash
        0,    // 5 = invalid
        0,    // 6 = invalid
        0,    // 7 = invalid
        0,    // 8 = invalid
        6500, // 9 = Fine weather
        6000, // 10 = Cloudy weather
        8000, // 11 = Shade
        // Values below from Wikipedia
        6430, // 12 = Daylight fluorescent (D 5700 - 7100K)
        6350, // 13 = Day white fluorescent (N 4600 - 5400K)
        4230, // 14 = Cool white fluorescent (W 3900 - 4500K)
        3450, // 15 = White fluorescent (WW 3200 - 3700K)
        0,    // 16 = Invalid
        2856, // 17 = Standard light A
        4874, // 18 = Standard light B
        6774, // 19 = Standard light C
        5503, // 20 = D55
        6504, // 21 = D65
        7504, // 22 = D75
        5003, // 23 = D50
        // Value from random internet searching
        3200, // 24 = ISO studio tungsten
        0     // > 25 = Other light source
    };
    const uint16_t DNG_TAG_CalibrationIlluminant_StdA = 17;
    const uint16_t DNG_TAG_CalibrationIlluminant_D65 = 21;

    const uint16_t EXIF_TAG_SensitivityType_Unknown = 0;
    const uint16_t EXIF_TAG_SensitivityType_SOS = 1;
    const uint16_t EXIF_TAG_SensitivityType_REI = 2;
    const uint16_t EXIF_TAG_SensitivityType_ISO = 3;
    const uint16_t EXIF_TAG_SensitivityType_SOS_REI = 4;
    const uint16_t EXIF_TAG_SensitivityType_SOS_ISO = 5;
    const uint16_t EXIF_TAG_SensitivityType_REI_ISO = 6;
    const uint16_t EXIF_TAG_SensitivityType_SOS_REI_ISO = 7;

}

#endif
