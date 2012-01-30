/* Copyright (c) 1995-2010, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/* Copyright (c) 2011, NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived 
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef FCAM_TEGRA_CAMERA_HAL
#define FCAM_TEGRA_CAMERA_HAL


/** \file
 * The main hal include file for FCam::Tegra. */

/* WARNING: This is the only hal header that the FCam
 * implementation will need to include. AVOID 
 * dependencies on any non-NDK Android headers.
 * Must include Time.h before include this file.
 */

/* Expected usage:
 * 1. Grab a reference to an IProduct interface
 *     IProduct *prod = System::openProduct();
 * 2. Open a CameraHal instance:
 *     ICamera *camhal = prod->openCameraHal(....);
 *     you need to pass an ICameraObserver object that
 *     implements two functions: onFrame(...) and 
 *     readyToCapture();
 * 
 */

namespace FCam { namespace Tegra { namespace Hal {

    enum {
        MAX_CAMERAS             =  6,
        MAX_SENSORMODES         = 16,
    };

    enum FrameFormat
    {
        YUV420p         = 0,
        RAW,
        UNKNOWN
    };

    enum StereoCamera
    {
        STEREO_LEFT     = 0,
        STEREO_RIGHT    = 1,
        STEREO_BOTH     = 2
    };

    struct CameraFrameParams
    {
        Time                   captureDoneTime;     // Time of StillCaptureProcessing event
        Time                   fillBufferDoneTime;  // Time of FillBufferDone
        Time				   processingDoneTime;	// Time ISP finished processing the frame
        int                    iso;                 // Queried iso at StillCaptureProcessing time
        int                    exposure;            // Queried exposure at StillCaptureProcessing time
        int                    wb;                  // Queried white balance at StillCaptureProcessing time
        int                    focusPos;            // focuser position at StillCaptureProcessing time
        int					   frameTime;			// Estimated frame time based on sensor mode.
    };

    struct CameraMode 
    {
       int  width;
       int  height;
       bool streaming;

       FrameFormat type;
       bool useGraphicBuffers;

       StereoCamera stereoMode;

       CameraMode(): width(0), height(0),
            streaming(false),
            type(UNKNOWN),
            useGraphicBuffers(false),
            stereoMode(STEREO_LEFT){};
    };

    struct Statistics
    {
        unsigned int meanBayer[4];
        unsigned int maxBayer[4];
        unsigned int clippedSensorLow;
        unsigned int clippedSensorHigh;
        unsigned int histogramBins;
        unsigned int histogramBayer[256][4];
    };

    struct CameraFrame 
    {
        unsigned char*   pBuffer;
        size_t  bufferSize;
        int     width;
        int     height;
        FrameFormat format;
        CameraFrameParams params;
    };

    // Each Tegra platform can have a different imager/focuser.
    // Define a set of structures to contain this information.
    // Platform.cpp will include the corresponding platform definitions 
    // according to the makefile

    /** A SensorConfig instance is returned to the Sensor. DO NOT
     *  use SensorConfig directly, use the Sensor functions instead. 
     *  There is no guarantee that this class will be supported on
     *  future versions of the FCam::Tegra API. */
    class SensorConfig
    {
        public:
            SensorConfig();
            struct ModeDesc
            {
                int width;
                int height;
                float fMinExposure;             // Min exposure in seconds
                float fMaxExposure;             // Max exposure in seconds
                float fMinFrameRate;            // Min frame rate
                float fMaxFrameRate;            // Max frame rate
            };

            ModeDesc modes[MAX_SENSORMODES];
            int      numberOfModes;

            float   fMinGain;
            float   fMaxGain;
            int     lensConfigIndex;            // Index of the lens config for this sensor
            int     exposureLatency;            // how many frames do we need to skip after setting the exposure
            int     gainLatency;                // how many frames do we need to skip after changing the gain.


            char   *psCameraName;               //
            int     cameraDirection;            // angle wrt to user control - 0 looks at user
    };

    /** A LensConfig instance is returned to the Lens. DO NOT
     *  use LensConfig directly, use the Lens member functions instead. 
     *  There is no guarantee that this class will be supported on
     *  future versions of the FCam::Tegra API. */
    class LensConfig
    {
    public:
        LensConfig();

        char*   psFocuserName;
        char*   psLensName;
        int     iFocusMinPosition;              // Min Tick
        int     iFocusMaxPosition;              // Max Tick
        float   fFarFocusDiopter;
        float   fNearFocusDiopter;
        float   fFocusDioptersPerTick;          // What is the change in diopters when we move the lens 1 tick
        float   fMinFocusSpeed;                 // Ticks per second
        float   fMaxFocusSpeed;                 // Ticks per second
        int     iFocusLatency;                  // In microseconds
        int     iFocusSettleTime;               // In microsecond
        float   fMinZoomFocalLength;            // In mm
        float   fMaxZoomFocalLength;            // In mm
        float   fMinZoomSpeed;
        float   fMaxZoomSpeed;
        int     iZoomLatency;
        float   fNarrowAperture;
        float   fWideAperture;
        float   fMinApertureSpeed;
        float   fMaxApertureSpeed;
        int     iApertureLatency;
    };

    class ICameraObserver
    {
    public:
        virtual void onFrame(CameraFrame* frame) = 0;
        virtual void readyToCapture() = 0;

        virtual ~ICameraObserver() {};
    };

    class ICamera
    {
    public:

        ICamera(ICameraObserver *pObserver, unsigned int id) {}

        virtual ~ICamera();

        virtual bool open() = 0;;
        virtual bool close() = 0;

        virtual const SensorConfig& getSensorConfig() = 0;
        virtual const LensConfig&   getLensConfig() = 0;

        /** Initiate a focuser change to position pos. Returns true if success, false otherwise*/
        virtual bool setFocuserPosition(int pos) = 0;

        /** Request the current focuser position */
        virtual bool getFocuserPosition(int *pos) = 0;

        /** Set sensor frame time in microsec - must fall in range of sensor's peak frame rate.
          * Use 0 for auto. In auto mode peak frame rate is favored by the camera driver, so exposure
          * time will be capped by this frame time*/
        virtual bool setSensorFrameTime(int frameTime) = 0;

        /** Get sensor frame time in microsec*/
        virtual bool getSensorFrameTime(int *frameTime) = 0;

        /** Set exposure time in microseconds. Use exposure -1 for AUTO */
        virtual bool setSensorExposure(int exposure) = 0;

        /** Get exposure time in microseconds. Sets exposure == -1 for  AUTO*/
        virtual bool getSensorExposure(int *exposure) = 0;

        /** Set the sensor gain. Use iso == -1 for AUTO */
        virtual bool setSensorEffectiveISO(int iso) = 0;
 
        /** Get the sensor gain. Set iso == -1 for AUTO */   
        virtual bool getSensorEffectiveISO(int *iso) = 0;

        /** Set whitebalance temperature in Kelvin */
        virtual bool setISPWhiteBalance(int whiteBalance) = 0;

        /** Get ISP statistics */
        virtual bool getISPStatistics(Statistics *stats) = 0;

        /** Sets up the capture for the current still image */
        virtual bool setCaptureMode(CameraMode m) = 0;

        virtual float setFlashForStillCapture(float brightness, int duration) = 0;

       /** Set the flash into torch mode with the given brightness. 
        *  A brightness of 0 will turn the torch off.
        *  A brightness greater than 0 will turn the torch on and leave
        *  it on until a new call comes to turn it off.
        *  Returns the actual intensity the flash was set to */
       virtual float setFlashTorchMode(float brightness) = 0;

       /** initiate single shot capture */
       virtual bool capture() = 0;
       virtual bool burstCapture(int numFrames) = 0;
       virtual bool startStreaming() = 0;
       virtual bool stopStreaming() = 0;

       virtual float fps() = 0;      
    };

    struct RendererFrame
    {
        void*   pBuffer;
        size_t  size;
        int     width;
        int     height;
        FrameFormat format;
        void*   pAPIPrivate;    // Private data to keep track of the frame
    };

    class IRenderer
    {
    public:
        IRenderer() {};
        virtual ~IRenderer() {};

        virtual bool open()  = 0;;
        virtual bool close() = 0;

        virtual bool setDisplaySurface(int nativeSurface) = 0;
        virtual RendererFrame* allocateFrame(unsigned int width, unsigned int height) = 0;
        virtual bool showFrame(RendererFrame* frame) = 0;
        virtual bool freeFrame(RendererFrame* frame) = 0;
    };


    class IProduct 
    {
    public:
        
        IProduct() {};
        virtual ~IProduct() {};

        virtual unsigned int numberOfCameras() = 0;

        virtual ICamera* getCameraHal(ICameraObserver *pObserver, unsigned int cameraNum) = 0;
        virtual void releaseCameraHal(ICamera* cameraHal) = 0;

        virtual IRenderer* getRendererHal() = 0;
        virtual void freeRendererHal(IRenderer* renderer) = 0;
    };

    class System 
    {
    public:
        static IProduct* openProduct();
        static void closeProduct(IProduct *product);

    private:
        static IProduct *pProduct;
        static int numProductRefs;
    };

}}}


#endif
