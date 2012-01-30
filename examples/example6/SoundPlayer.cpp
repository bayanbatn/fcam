#include <assert.h>
#include "SoundPlayer.h"

// engine interfaces
static unsigned int engineRefs = 0;
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;

/** \file */

/***************************************************************/
/* SoundPlayer implementation                                  */
/***************************************************************/

/* SoundPlayer constructor */
SoundPlayer::SoundPlayer(AAssetManager* mgr):
    mgr(mgr),
    fdPlayerObject(NULL),
    fdPlayerPlay(NULL),
    fdPlayerSeek(NULL),
    fdPlayerMuteSolo(NULL),
    fdPlayerVolume(NULL)
{
    if ((engineRefs == 0) && createEngine())
    {
        acquiredEngineRef = true;
        engineRefs++;
    }
    else
    {
        acquiredEngineRef = true;
        engineRefs++;
    }
}

/* SoundPlayer destructor */
SoundPlayer::~SoundPlayer()
{
    // destroy file descriptor audio player object, and invalidate all associated interfaces
    if (fdPlayerObject != NULL) {
        (*fdPlayerObject)->Destroy(fdPlayerObject);
        fdPlayerObject = NULL;
        fdPlayerPlay = NULL;
        fdPlayerSeek = NULL;
        fdPlayerMuteSolo = NULL;
        fdPlayerVolume = NULL;
    }

    if (acquiredEngineRef)
    {
        engineRefs--;
        if (engineRefs == 0)
        {
            destroyEngine();
        }
    }
}

/* Play a buffer */
bool SoundPlayer::playAsset(const char *assetname)
{
    SLresult result;

    // destroy file descriptor audio player object, and invalidate all associated interfaces
    if (fdPlayerObject != NULL) {
        (*fdPlayerObject)->Destroy(fdPlayerObject);
        fdPlayerObject = NULL;
        fdPlayerPlay = NULL;
        fdPlayerSeek = NULL;
        fdPlayerMuteSolo = NULL;
        fdPlayerVolume = NULL;
    }

    assert(NULL != mgr);
    AAsset* asset = AAssetManager_open(mgr, assetname, AASSET_MODE_UNKNOWN);

    // the asset might not be found
    if (NULL == asset) {
        return false;
    }

    // open asset as file descriptor
    off_t start, length;
    int fd = AAsset_openFileDescriptor(asset, &start, &length);
    assert(0 <= fd);
    AAsset_close(asset);

    // configure audio source
    SLDataLocator_AndroidFD loc_fd = {SL_DATALOCATOR_ANDROIDFD, fd, start, length};
    SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
    SLDataSource audioSrc = {&loc_fd, &format_mime};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_SEEK, SL_IID_MUTESOLO, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &fdPlayerObject, &audioSrc, &audioSnk,
            3, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the player
    result = (*fdPlayerObject)->Realize(fdPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the play interface
    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_PLAY, &fdPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);

    // get the seek interface
    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_SEEK, &fdPlayerSeek);
    assert(SL_RESULT_SUCCESS == result);

    // get the mute/solo interface
    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_MUTESOLO, &fdPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);

    // get the volume interface
    result = (*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_VOLUME, &fdPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);

    // set the player's state
    result = (*fdPlayerPlay)->SetPlayState(fdPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);

    return (result == SL_RESULT_SUCCESS);
}

int SoundPlayer::getLatency()
{
    return 0;
}


bool SoundPlayer::createEngine()
{
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    return (result == SL_RESULT_SUCCESS);
}

bool SoundPlayer::destroyEngine()
{
    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    return true;
}


/***************************************************************/
/* SoundPlayer::SoundAction implementation                     */
/***************************************************************/

/* SoundAction constructors */
SoundPlayer::SoundAction::SoundAction(SoundPlayer * a) {
    player = a;
    time = 0;
    latency = a ? a->getLatency() : 0;
}

SoundPlayer::SoundAction::SoundAction(SoundPlayer * a, int t) {
    player = a;
    time = t;
    latency = a ? a->getLatency() : 0;
}

SoundPlayer::SoundAction::SoundAction(const SoundPlayer::SoundAction & b) {
    // Copy fields from the target.
    time = b.time;
    latency = b.latency;
    player = b.getPlayer();
    assetname = b.assetname;
}

/* SoundAction destructor */
SoundPlayer::SoundAction::~SoundAction() {
}

void SoundPlayer::SoundAction::setAsset(const char* asset) {
    assetname = asset;
}

/* Perform the required action */
void SoundPlayer::SoundAction::doAction() {
    player->playAsset(assetname.c_str());
}



