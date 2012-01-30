#ifndef FCAM_BEEPER_H
#define FCAM_BEEPER_H

/** \file */

#include <string>

#include <FCam/FCam.h>
#include <FCam/Action.h>
#include <FCam/Device.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


/*
 * A synchronized beeper example. As a device,
 * it inherits from FCam::Device, and declares
 * nested classes that inherit from CopyableAction
 */
class SoundPlayer : public FCam::Device {
    
public:
    

    SoundPlayer(AAssetManager* mgr);
    ~SoundPlayer();
    
    /*
     * An action representing the playback of a .WAV file.
     */
    class SoundAction : public FCam::CopyableAction<SoundAction> {
    public:

        /* The enum to return as type() */
        enum {
            SoundPlay = CustomAction + 1,
        };

        /* Constructors and destructor */
        ~SoundAction();
        SoundAction(SoundPlayer * b);
        SoundAction(SoundPlayer * b, int time);
        SoundAction(const SoundAction &);
        
        /* Implementation of doAction() as required */
        void doAction();
        
        /* Load the specified file into buffer and prepares playback */
        void setAsset(const char *asset);
        
        /* Return the underlying device */
        SoundPlayer * getPlayer() const { return player; }

        int type() const { return SoundPlay; }

    protected:
        
        SoundPlayer * player;
        std::string assetname;
    };
    
    /* Normally, this is where a device would add metadata tags to a
     * just-created frame , based on the timestamps in the
     * Frame. However, we don't have anything useful to add here, so
     * tagFrame does nothing. */
    void tagFrame(FCam::Frame) {}
    
    /* Play an application asset*/
    bool playAsset(const char *asset);
    
    /* Returns latency in microseconds */
    int getLatency();
    
    void handleEvent(const FCam::Event&) {};

protected:

    static bool createEngine();
    static bool destroyEngine();

    // Asset manager..
    AAssetManager* mgr;

    // Acquired an engine ref
    bool acquiredEngineRef;

    // file descriptor player interfaces
    SLObjectItf fdPlayerObject;
    SLPlayItf fdPlayerPlay;
    SLSeekItf fdPlayerSeek;
    SLMuteSoloItf fdPlayerMuteSolo;
    SLVolumeItf fdPlayerVolume;
};

#endif
