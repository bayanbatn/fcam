#define GL_GLEXT_PROTOTYPES
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <FCam/Tegra.h>
#include "FCamInterface.h"
#include "AsyncImageWriter.h"
#include "MyAutoFocus.h"
#include "ParamStat.h"
#include "HPT.h"

#define PREVIEW_IMAGE_WIDTH  640
#define PREVIEW_IMAGE_HEIGHT 480

// the size of the patch used in local white balancing
#define TOUCH_PATCH_SIZE     15

#define CAPTURE_IMAGE_WIDTH  2592
#define CAPTURE_IMAGE_HEIGHT 1936

#define PI_PLANE_SIZE (PREVIEW_IMAGE_WIDTH*PREVIEW_IMAGE_HEIGHT)
#define PI_U_OFFSET   (PREVIEW_IMAGE_WIDTH*PREVIEW_IMAGE_HEIGHT)
#define PI_V_OFFSET   (PREVIEW_IMAGE_WIDTH*PREVIEW_IMAGE_HEIGHT+(PREVIEW_IMAGE_WIDTH*PREVIEW_IMAGE_HEIGHT>>2))

#define FPS_UPDATE_PERIOD 500 // in ms

/* GL_OES_egl_image_external */
#ifndef GL_OES_egl_image_external
#define GL_OES_egl_image_external 1
#define GL_TEXTURE_EXTERNAL_OES                         0x8D65
#define GL_SAMPLER_EXTERNAL_OES                         0x8D66
#define GL_TEXTURE_BINDING_EXTERNAL_OES                 0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES             0x8D68
#endif

// ==========================================================================================
// NATIVE APP STATIC DATA
// ==========================================================================================

static FCAM_INTERFACE_DATA *sAppData;

static void *FCamAppThread(void *tdata);

// ==========================================================================================
// PUBLIC JNI FUNCTIONS
// ==========================================================================================

extern "C" {

JNIEXPORT int JNICALL Java_com_nvidia_fcamerapro_FCamInterface_lockViewerTexture(JNIEnv *env, jobject thiz) {
	FCam::Tegra::Hal::SharedBuffer *buffer = sAppData->tripleBuffer->swapFrontBuffer();

	GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, tid);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES) buffer->getEGLImage());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	sAppData->viewerBufferTexId = tid;

	return tid;
}

JNIEXPORT int JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getViewerTextureTarget(JNIEnv *env, jobject thiz) {
	return GL_TEXTURE_EXTERNAL_OES;
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_unlockViewerTexture(JNIEnv *env, jobject thiz) {
    glDeleteTextures(1, &sAppData->viewerBufferTexId);
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamInt(JNIEnv *env, jobject thiz, jint param, jint value) {
	sAppData->requestQueue.produce(ParamSetRequest(param, &value, sizeof(int)));
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamIntArray(JNIEnv *env, jobject thiz, jint param, jintArray value) {
	int arraySize;
	int *arrayData;

	int paramId = param & 0xffff;
	int pictureId = param >> 16;

	switch (paramId) {
	case PARAM_SHOT:
		arraySize = env->GetArrayLength(value);
		if (arraySize != 5) {
			ERROR("setParamIntArray(PARAM_SHOT): incorrect array size!");
			return;
		}

		arrayData = env->GetIntArrayElements(value, 0);
		sAppData->requestQueue.produce(ParamSetRequest(param, arrayData, arraySize * sizeof(int)));
		env->ReleaseIntArrayElements(value, arrayData, 0);
		break;

	default:
		ERROR("setParamIntArray(%i): received unsupported param id!", paramId);
	}
}

JNIEXPORT int JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamInt(JNIEnv *env, jobject thiz, jint param) {
	pthread_mutex_lock(&sAppData->currentShotLock);

	FCAM_SHOT_PARAMS *previousShot = &sAppData->previousShot;
	int rval = -1;

	int paramId = param & 0xffff;
	int pictureId = param >> 16;

	switch (paramId) {
	case PARAM_PREVIEW_EXPOSURE:
		rval = previousShot->preview.autoExposure ? previousShot->preview.evaluated.exposure : previousShot->preview.user.exposure;
		break;
	case PARAM_PREVIEW_FOCUS:
		rval = previousShot->preview.autoFocus ? previousShot->preview.evaluated.focus : previousShot->preview.user.focus;
		break;
	case PARAM_PREVIEW_GAIN:
		rval = previousShot->preview.autoGain ? previousShot->preview.evaluated.gain : previousShot->preview.user.gain;
		break;
	case PARAM_PREVIEW_WB:
		rval = previousShot->preview.autoWB ? previousShot->preview.evaluated.wb : previousShot->preview.user.wb;
		break;
	case PARAM_BURST_SIZE:
		rval = previousShot->burstSize;
		break;
	case PARAM_OUTPUT_FORMAT:
		rval = previousShot->outputFormat;
		break;
	case PARAM_VIEWER_ACTIVE:
		rval = sAppData->isViewerActive;
		break;
	case PARAM_TAKE_PICTURE:
		rval = sAppData->isCapturing;
		break;
	default:
		ERROR("received unsupported param id (%i)!", paramId);
	}

	pthread_mutex_unlock(&sAppData->currentShotLock);

	return rval;
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamIntArray(JNIEnv *env, jobject thiz, jint param, jintArray value) {
	int arraySize;
	int *arrayData;

	pthread_mutex_lock(&sAppData->currentShotLock);

	int paramId = param & 0xffff;
	int pictureId = param >> 16;

	switch (paramId) {
	case PARAM_SHOT:
		arraySize = env->GetArrayLength(value);
		if (arraySize != 5) {
			ERROR("getParamIntArray(PARAM_SHOT): incorrect shot array size!");
			return;
		}

		arrayData = env->GetIntArrayElements(value, 0);

		arrayData[SHOT_PARAM_EXPOSURE] = sAppData->currentShot.captureSet[pictureId].exposure;
		arrayData[SHOT_PARAM_FOCUS] = sAppData->currentShot.captureSet[pictureId].focus;
		arrayData[SHOT_PARAM_GAIN] = sAppData->currentShot.captureSet[pictureId].gain;
		arrayData[SHOT_PARAM_WB] = sAppData->currentShot.captureSet[pictureId].wb;
		arrayData[SHOT_PARAM_FLASH] = sAppData->currentShot.captureSet[pictureId].flashOn;

		env->ReleaseIntArrayElements(value, arrayData, 0);
		break;

	default:
		ERROR("getParamIntArray(%i): received unsupported param id!", paramId);
	}

	pthread_mutex_unlock(&sAppData->currentShotLock);
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamFloat(JNIEnv *env, jobject thiz, jint param, jfloat value) {
	sAppData->requestQueue.produce(ParamSetRequest(param, &value, sizeof(float)));
}

JNIEXPORT float JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamFloat(JNIEnv *env, jobject thiz, jint param) {
	float rval = -1.0f;

	int paramId = param & 0xffff;
	int pictureId = param >> 16;

	switch (paramId) {
	case PARAM_CAPTURE_FPS:
		rval = sAppData->captureFps;
		break;
	default:
		ERROR("getParamFloat(%i): received unsupported param id!", paramId);
	}

	return rval;
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamFloatArray(JNIEnv *env, jobject thiz, jint param, jfloatArray value) {
	int arraySize;
	float *arrayData;

	pthread_mutex_lock(&sAppData->currentShotLock);

	int paramId = param & 0xffff;
	int pictureId = param >> 16;

	switch (paramId) {
	case PARAM_LUMINANCE_HISTOGRAM:
		arraySize = env->GetArrayLength(value);
		if (arraySize != HISTOGRAM_SIZE) {
			ERROR("getParamFloatArray(PARAM_LUMINANCE_HISTOGRAM): incorrect array size!");
			return;
		}

		arrayData = env->GetFloatArrayElements(value, 0);

		for (int i = 0; i < HISTOGRAM_SIZE; i++) {
			arrayData[i] = sAppData->previousShot.histogramData[i];
		}

		env->ReleaseFloatArrayElements(value, arrayData, 0);
		break;

	default:
		ERROR("getParamFloatArray(%i): received unsupported param id!", paramId);
	}

	pthread_mutex_unlock(&sAppData->currentShotLock);
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamString(JNIEnv *env, jobject thiz, jint param, jstring value) {
    const char *str = (const char *) env->GetStringUTFChars(value, 0);
	sAppData->requestQueue.produce(ParamSetRequest(param, str, strlen(str) + 1));
    env->ReleaseStringUTFChars(value, str);
}

JNIEXPORT jstring JNICALL FCamInterface_getParamString(JNIEnv *env, jobject thiz, jint param) {
	return 0;
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_init(JNIEnv *env, jobject thiz) {
	sAppData->fcamInstanceRef = env->NewGlobalRef(thiz);

	// launch the work thread
	pthread_create(&sAppData->appThread, 0, FCamAppThread, sAppData);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
	JNIEnv *env;

	LOG("JNI_OnLoad called");
	if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOG("Failed to get the environment using GetEnv()");
		return -1;
	}

	jclass fcamClassRef = env->FindClass("com/nvidia/fcamerapro/FCamInterface");

	// Initialize sAppData (camera thread state) and attach hooks to some methods on JAVA side.
	sAppData = new FCAM_INTERFACE_DATA;
	sAppData->javaVM = vm;
	sAppData->notifyCaptureStart = env->GetMethodID(fcamClassRef, "notifyCaptureStart", "()V");
	sAppData->notifyCaptureComplete = env->GetMethodID(fcamClassRef, "notifyCaptureComplete", "()V");
	sAppData->notifyFileSystemChange = env->GetMethodID(fcamClassRef, "notifyFileSystemChange", "()V");
	sAppData->fcamClassRef = env->NewGlobalRef(fcamClassRef);
	env->DeleteLocalRef(fcamClassRef);

	// Initialize tripple buffer.
	for (unsigned int i = 0; i < 3; i++)
		sAppData->viewBuffers[i] = new FCam::Tegra::Hal::SharedBuffer(PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);
	sAppData->tripleBuffer = new TripleBuffer<FCam::Tegra::Hal::SharedBuffer>(sAppData->viewBuffers);

	// Initialize default shot parameters.
	memset(&sAppData->currentShot, 0, sizeof(FCAM_SHOT_PARAMS));
	sAppData->currentShot.preview.user.exposure = 30000; // 30ms
	sAppData->currentShot.preview.user.gain = 1;
	sAppData->currentShot.preview.user.wb = 6500;
	sAppData->currentShot.preview.user.focus = 10;
	sAppData->currentShot.preview.user.flashOn = 0;
	memcpy(&sAppData->previousShot, &sAppData->currentShot, sizeof(FCAM_SHOT_PARAMS));
	pthread_mutex_init(&sAppData->currentShotLock, 0);

	// Set flags
	sAppData->isCapturing = false;
	sAppData->isViewerActive = false;
	sAppData->isGLInitDone = false;

	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
	JNIEnv *env;

	LOG("JNI_OnLoad called");
	if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOG("Failed to get the environment using GetEnv()");
		return;
	}

	env->DeleteGlobalRef(sAppData->fcamClassRef);
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_enqueueMessageForAutofocus(JNIEnv *env, jobject thiz) {
	/* [CS478]
	 * Enqueue a new message that represents a request for global autofocus.
	 */
	// TODO TODO TODO
	// TODO TODO TODO
	// TODO TODO TODO
	// TODO TODO TODO
	// TODO TODO TODO
}

JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_enqueueMessageForAutofocusSpot(JNIEnv *env, jobject thiz, jfloat x, jfloat y) {
	/* [CS478]
	 * Enqueue a new message that represents a request for local autofocus
	 */
	// TODO TODO TODO
	// TODO TODO TODO
	// TODO TODO TODO
	// TODO TODO TODO
	// TODO TODO TODO
}

}

// ==========================================================================================
// FCAM NATIVE THREAD
// ==========================================================================================

static void OnFileSystemChanged(void) {
	// Called from the asynchronous image writer thread -> queue request to resolve in the main app thread
	const int value = 1;
	sAppData->requestQueue.produce(ParamSetRequest(PARAM_PRIV_FS_CHANGED, &value, sizeof(int)));
}

static void OnCapture(FCAM_INTERFACE_DATA *tdata, AsyncImageWriter *writer, FCam::Tegra::Sensor &sensor,
		FCam::Tegra::Flash &flash, FCam::Tegra::Lens &lens) {
	FCAM_SHOT_PARAMS *currentShot = &tdata->currentShot;
	FCAM_SHOT_PARAMS *previousShot = &tdata->previousShot;

	// Stop streaming and drain frames. It should not be necessary, but let's be safe.
	sensor.stopStreaming();
	while (sensor.shotsPending() > 0) {
		sensor.getFrame();
	}

	// Prepare a new image set.
	ImageSet *is = writer->newImageSet();

	// Prepare flash action.
	FCam::Flash::FireAction flashAction(&flash);
    flashAction.time = 0;
    flashAction.brightness = flash.maxBrightness();

	// Request capture for each shot.
	for (int i = 0; i < currentShot->burstSize; i++) {
	    FCam::Shot shot;
	    shot.exposure = currentShot->captureSet[i].exposure;
	    shot.gain = currentShot->captureSet[i].gain;
	    shot.whiteBalance = currentShot->captureSet[i].wb;
	    shot.image = FCam::Image(CAPTURE_IMAGE_WIDTH, CAPTURE_IMAGE_HEIGHT, FCam::YUV420p);
	    shot.histogram.enabled = false;
	    shot.sharpness.enabled = false;
	    if (currentShot->captureSet[i].flashOn != 0) {
	        shot.addAction(flashAction);
	    }
	    sensor.capture(shot);
	}

    // Currently we pause capture while writing the file. It may be good
	// to change this in the future so that writing occurs in the background.
    FileFormatDescriptor fmt(FileFormatDescriptor::EFormatJPEG, 95);
	while (sensor.shotsPending() > 0) {
		is->add(fmt, sensor.getFrame());
	}
	writer->push(is);
}

// This method is the main workhorse, and is run by the camera thread.
static void *FCamAppThread(void *ptr) {
	FCAM_INTERFACE_DATA *tdata = (FCAM_INTERFACE_DATA *)ptr;
	Timer timer;
    JNIEnv *env;
    tdata->javaVM->AttachCurrentThread(&env, 0);
    AsyncImageWriter *writer = 0; // Initialized on the first PARAM_OUTPUT_DIRECTORY set request.

    // Initialize FCam devices.
    FCam::Tegra::Sensor sensor;
    FCam::Tegra::Lens lens;
    FCam::Tegra::Flash flash;
    sensor.attach(&lens);
    sensor.attach(&flash);
    MyAutoFocus autofocus(&lens);

    FCam::Image previewImage(PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT, FCam::YUV420p);
    FCam::Tegra::Shot shot;

    // Initialize FPS stat calculation.
    tdata->captureFps = 30; // assuming 30hz
    double fpsUpdateTime = timer.get();
    int frameCount = 0;

    // Local task queue that processes messages from the Android application.
    std::queue<ParamSetRequest> taskQueue;
	ParamSetRequest task;

	for (;;) {
		FCAM_SHOT_PARAMS *currentShot = &tdata->currentShot;
		FCAM_SHOT_PARAMS *previousShot = &tdata->previousShot;
	    // Copy tasks to local queue
	    sAppData->requestQueue.consumeAll(taskQueue);

	    // Parse all tasks from the Android applications.
	    while (!taskQueue.empty()) {
	    	task = taskQueue.front(); taskQueue.pop();

			bool prevValue;
			int taskId = task.getId() & 0xffff;
			int *taskData = (int *)task.getData();
			int pictureId = task.getId() >> 16;

			switch (taskId) {
			case PARAM_SHOT:
				// Note: Exposure is bounded below at 1/1000 (FCam bug?)
				currentShot->captureSet[pictureId].exposure = taskData[SHOT_PARAM_EXPOSURE] < 1000 ? 1000 : taskData[SHOT_PARAM_EXPOSURE];
				currentShot->captureSet[pictureId].focus = taskData[SHOT_PARAM_FOCUS];
				currentShot->captureSet[pictureId].gain = taskData[SHOT_PARAM_GAIN];
				currentShot->captureSet[pictureId].wb = taskData[SHOT_PARAM_WB];
				currentShot->captureSet[pictureId].flashOn = taskData[SHOT_PARAM_FLASH];
				break;
			case PARAM_PREVIEW_EXPOSURE:
				currentShot->preview.user.exposure = taskData[0];
				break;
			case PARAM_PREVIEW_FOCUS:
				currentShot->preview.user.focus = taskData[0];
				break;
			case PARAM_PREVIEW_GAIN:
				currentShot->preview.user.gain = taskData[0];
				break;
			case PARAM_PREVIEW_WB:
				currentShot->preview.user.wb = taskData[0];
				break;
			case PARAM_PREVIEW_AUTO_EXPOSURE_ON:
				prevValue = currentShot->preview.autoExposure;
				currentShot->preview.autoExposure = taskData[0] != 0;
				if (!prevValue && prevValue ^ currentShot->preview.autoExposure != 0) {
					previousShot->preview.evaluated.exposure = currentShot->preview.user.exposure;
				} else {
					currentShot->preview.user.exposure = previousShot->preview.evaluated.exposure;
				}
				break;
			case PARAM_PREVIEW_AUTO_FOCUS_ON:
				prevValue = currentShot->preview.autoFocus;
				currentShot->preview.autoFocus = taskData[0] != 0;
				if (!prevValue && prevValue ^ currentShot->preview.autoFocus != 0) {
					previousShot->preview.evaluated.focus = currentShot->preview.user.focus;
				} else {
					currentShot->preview.user.focus = previousShot->preview.evaluated.focus;
				}
				break;
			case PARAM_PREVIEW_AUTO_GAIN_ON:
				prevValue = currentShot->preview.autoGain;
				currentShot->preview.autoGain = taskData[0] != 0;
				if (!prevValue && prevValue ^ currentShot->preview.autoGain != 0) {
					previousShot->preview.evaluated.gain = currentShot->preview.user.gain;
				} else {
					currentShot->preview.user.gain = previousShot->preview.evaluated.gain;
				}
				break;
			case PARAM_PREVIEW_AUTO_WB_ON:
				prevValue = currentShot->preview.autoWB;
				currentShot->preview.autoWB = taskData[0] != 0;
				if (!prevValue && prevValue ^ currentShot->preview.autoWB != 0) {
					previousShot->preview.evaluated.wb = currentShot->preview.user.wb;
				} else {
					currentShot->preview.user.wb = previousShot->preview.evaluated.wb;
				}
				break;
			case PARAM_RESOLUTION:
				break;
			case PARAM_BURST_SIZE:
				currentShot->burstSize = taskData[0];
				break;
			case PARAM_OUTPUT_FORMAT:
				break;
			case PARAM_VIEWER_ACTIVE:
				tdata->isViewerActive = taskData[0] != 0;
				break;
			case PARAM_OUTPUT_DIRECTORY:
				if (writer == 0) {
					writer = new AsyncImageWriter((char *)task.getData());
				    writer->setOnFileSystemChangedCallback(OnFileSystemChanged);
				}
				break;
			case PARAM_OUTPUT_FILE_ID:
				AsyncImageWriter::SetFreeFileId(taskData[0]);
				break;
			case PARAM_TAKE_PICTURE:
				if (writer != 0 && task.getDataAsInt() != 0) { // Don't take picture if we can't write out.
					// capture begin
					tdata->isCapturing = true;
					// notify capture start
					env->CallVoidMethod(tdata->fcamInstanceRef, tdata->notifyCaptureStart);
					OnCapture(tdata, writer, sensor, flash, lens);
					// capture done
					tdata->isCapturing = false;
					// notify capture completion
					env->CallVoidMethod(tdata->fcamInstanceRef, tdata->notifyCaptureComplete);
				}
				break;
			case PARAM_PRIV_FS_CHANGED:
				if (taskData[0] != 0) {
					// notify fs change
					env->CallVoidMethod(tdata->fcamInstanceRef, tdata->notifyFileSystemChange);
				}
				break;
				/* [CS478]
				 * You will probably want extra cases here, to handle messages
				 * that request autofocus to be activated. Define any new
				 * message types in ParamSetRequestion.h.
				 */
				// TODO TODO TODO
				// TODO TODO TODO
				// TODO TODO TODO
				// TODO TODO TODO
				// TODO TODO TODO
			default:
				ERROR("TaskDispatch(): received unsupported task id (%i)!", taskId);
			}
	    }

		if (!tdata->isViewerActive) continue; // Viewer is inactive, so skip capture.

		// Setup preview shot parameters.
	    shot.exposure = currentShot->preview.autoExposure ? previousShot->preview.evaluated.exposure : currentShot->preview.user.exposure;
	    shot.gain = currentShot->preview.autoGain ? previousShot->preview.evaluated.gain : currentShot->preview.user.gain;
	    shot.whiteBalance = currentShot->preview.autoWB ? previousShot->preview.evaluated.wb : currentShot->preview.user.wb;
	    shot.image = previewImage;
	    shot.histogram.enabled = true;
	    shot.histogram.region = FCam::Rect(0, 0, PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);
	    shot.sharpness.enabled = currentShot->preview.autoFocus;
	    shot.sharpness.size = FCam::Size(16, 12);
	    shot.fastMode = true;
	    shot.clearActions();

	    // If in manual focus mode, and the lens is not at the right place, add an action to move it.
	    if (!currentShot->preview.autoFocus && previousShot->preview.user.focus != currentShot->preview.user.focus) {
	    	shot.clearActions();
            FCam::Lens::FocusAction focusAction(&lens);
            focusAction.time = 0;
            focusAction.focus = currentShot->preview.user.focus;
            shot.addAction(focusAction);
	    }

	    // Send the shot request to FCam.
	    sensor.stream(shot);

	    // Fetch the incoming frame from FCam.
	    FCam::Frame frame = sensor.getFrame();

	    // Process the incoming frame. If autoExposure or autoGain is enabled, update parameters based on the frame.
	    if (currentShot->preview.autoExposure || currentShot->preview.autoGain) {
	    	FCam::autoExpose(&shot, frame, sensor.maxGain(), sensor.maxExposure(), sensor.minExposure(), 0.3);
	    	currentShot->preview.evaluated.exposure = shot.exposure;
	    	currentShot->preview.evaluated.gain = shot.gain;
	    }

	    // Process the incoming frame. If autoWB is enabled, update parameters based on the frame.
	    if (currentShot->preview.autoWB) {
            FCam::autoWhiteBalance(&shot, frame);
            currentShot->preview.evaluated.wb = shot.whiteBalance;
	    }

	    /* [CS478]
	     * You should process the incoming frame for autofocus, if necessary.
	     * Your autofocus (MyAutoFocus.h) has a function called update(...).
	     */
	    // TODO TODO TODO
	    // TODO TODO TODO
	    // TODO TODO TODO
	    // TODO TODO TODO
	    // TODO TODO TODO

	    // Update histogram data
	    const FCam::Histogram &histogram = frame.histogram();
	    int maxBinValue = 1;
	    for (int i = 0; i < 64; i++) {
	    	int currBinValue = histogram(i);
	    	maxBinValue = (currBinValue > maxBinValue) ? currBinValue : maxBinValue;
	    	currentShot->histogramData[i * 4] = currBinValue;
	    }
	    float norm = 1.0f / maxBinValue;
	    for (int i = 0; i < 64; i++) {
	    	currentShot->histogramData[i * 4] *= norm;
	    	currentShot->histogramData[i * 4 + 1] = 0.0f;
	    	currentShot->histogramData[i * 4 + 2] = 0.0f;
	    	currentShot->histogramData[i * 4 + 3] = 0.0f;
	    }

	    // Update the frame buffer.
	    uchar *src = (uchar *)frame.image()(0, 0);
	    FCam::Tegra::Hal::SharedBuffer *captureBuffer = tdata->tripleBuffer->getBackBuffer();
	    uchar *dest = (uchar *)captureBuffer->lock();

	    // Note: why do we need to shuffle U and V channels? It seems to be a bug.
	    memcpy(dest, src, PI_PLANE_SIZE);
	    memcpy(dest + PI_U_OFFSET, src + PI_V_OFFSET, PI_PLANE_SIZE >> 2);
	    memcpy(dest + PI_V_OFFSET, src + PI_U_OFFSET, PI_PLANE_SIZE >> 2);
	    captureBuffer->unlock();
	    tdata->tripleBuffer->swapBackBuffer();

	    // Frame capture complete, copy current shot data to previous one
	    pthread_mutex_lock(&tdata->currentShotLock);
	    memcpy(&tdata->previousShot, &tdata->currentShot, sizeof(FCAM_SHOT_PARAMS));
	    pthread_mutex_unlock(&tdata->currentShotLock);
	    frameCount++;

    	// Update FPS
	    double time = timer.get();
	    double dt = time - fpsUpdateTime;
	    if (dt > FPS_UPDATE_PERIOD) {
	    	float fps = frameCount * (1000.0 / dt);
	    	fpsUpdateTime = time;
	    	frameCount = 0;
	    	tdata->captureFps = fps;
	    }
	}

    tdata->javaVM->DetachCurrentThread();

    // delete instance ref
    env->DeleteGlobalRef(tdata->fcamInstanceRef);

	return 0;
}

