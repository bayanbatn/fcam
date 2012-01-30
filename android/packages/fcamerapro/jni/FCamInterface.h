#ifndef _FCAMINTERFACE_H
#define _FCAMINTERFACE_H

#include <jni.h>
#include <string>
#include "WorkQueue.h"
#include "TripleBuffer.h"
#include "ParamSetRequest.h"
#include "FCam/Tegra/hal/SharedBuffer.h"

#define FCAM_MAX_PICTURES_PER_SHOT 16

typedef struct S_FCAM_AUTO_PARAMS {
	int exposure, focus, gain, wb, flashOn;
} FCAM_CAPTURE_PARAMS;

typedef struct S_FCAM_SHOT_PARAMS {
	struct {
		FCAM_CAPTURE_PARAMS evaluated;
		FCAM_CAPTURE_PARAMS user;
		bool autoExposure, autoFocus, autoGain, autoWB;
	} preview;
	FCAM_CAPTURE_PARAMS captureSet[FCAM_MAX_PICTURES_PER_SHOT];
	int outputFormat;
	int burstSize;
	float histogramData[HISTOGRAM_SIZE];
} FCAM_SHOT_PARAMS;

typedef struct S_FCAM_NATIVE_THREAD_DATA {
	JavaVM *javaVM;
	jobject fcamInstanceRef;
	jobject fcamClassRef;
	jmethodID notifyCaptureStart;
	jmethodID notifyCaptureComplete;
	jmethodID notifyFileSystemChange;

	pthread_t appThread;

	FCam::Tegra::Hal::SharedBuffer *viewBuffers[3];
	TripleBuffer<FCam::Tegra::Hal::SharedBuffer> *tripleBuffer;
	uint viewerBufferTexId;

	WorkQueue<ParamSetRequest> requestQueue;

	FCAM_SHOT_PARAMS previousShot, currentShot;
	pthread_mutex_t currentShotLock;
	std::string nextShootFileName;
	float captureFps;
	bool isCapturing, isViewerActive;
	bool isGLInitDone;
} FCAM_INTERFACE_DATA;

#endif

