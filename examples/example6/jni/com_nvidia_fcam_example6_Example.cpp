#include <jni.h>
#include <pthread.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>


extern int main(int argc, char* argv[]);

/* A structure that holds the methods ID of the Java functions that the app
 * can call */
struct fields_t {
    jmethodID   completionCb;     // example has finished executing.
    jmethodID   printCb;          // A callback to send text to the app to print.
};

static JavaVM  *gJavaVM;
static jclass   exampleJClass = NULL;
static jobject  exampleInstance = NULL;
static jobject  assetManagerRef = NULL;
AAssetManager* nativeAssetManager = NULL;

static fields_t fields;
static pthread_t fcamthread;

void jniAttachThread()
{
    JNIEnv *env;
    int status = gJavaVM->AttachCurrentThread(&env, NULL);   
}

void jniDetachThread()
{
    gJavaVM->DetachCurrentThread();
}

void notifyCompletion()
{
    JNIEnv * env;
    gJavaVM->GetEnv((void**) &env, JNI_VERSION_1_4);
    env->CallVoidMethod(exampleInstance, fields.completionCb);
}

void appPrint(char *str)
{
    JNIEnv * env;

    gJavaVM->GetEnv((void**) &env, JNI_VERSION_1_4);
    jstring text = env->NewStringUTF(str);
    env->CallVoidMethod(exampleInstance, fields.printCb, text);
}

void deleteGlobalRefs()
{
    JNIEnv * env;
    gJavaVM->GetEnv((void**) &env, JNI_VERSION_1_4);

    if (exampleJClass != NULL)
    {
        env->DeleteGlobalRef(exampleJClass);
        exampleJClass = NULL;
    }

    if (exampleInstance != NULL)
    {
        env->DeleteGlobalRef(exampleInstance);
        exampleInstance = NULL;
    }

    if (assetManagerRef != NULL)
    {
        env->DeleteGlobalRef(assetManagerRef);
        assetManagerRef = NULL;
        nativeAssetManager = NULL;
    }
}


#ifdef __cplusplus
extern "C" {
#endif

jint JNI_OnLoad(JavaVM* vm, void* reserved) 
{
    // Cache the java vm
    gJavaVM = vm;
    return JNI_VERSION_1_4;
}

void *fcam_thread_(void *arg) {
    jniAttachThread();
    main(0, NULL);
    notifyCompletion();
    deleteGlobalRefs();
    jniDetachThread();
    pthread_exit(NULL);
    return NULL;
}

/*
 * Class:     com_nvidia_fcam_example1_Example
 * Method:    run
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_nvidia_fcam_example6_Example_run
  (JNIEnv *env, jobject thiz)
{

    // Register the Java based instance, class and methods we need to later call.
    exampleInstance = env->NewGlobalRef(thiz);
    exampleJClass   = (jclass)env->NewGlobalRef(env->GetObjectClass(thiz));
    fields.completionCb = env->GetMethodID(exampleJClass, "onCompletion", "()V");
    fields.printCb = env->GetMethodID(exampleJClass, "printFromNative", "(Ljava/lang/String;)V");

    // Launch the fcamera thread
    pthread_create(&fcamthread, NULL, fcam_thread_, NULL);

}

JNIEXPORT jboolean JNICALL Java_com_nvidia_fcam_example6_Example_setAssets
  (JNIEnv* env, jobject thiz, jobject assetManager)
{

    nativeAssetManager = AAssetManager_fromJava(env, assetManager);

    // Reference the asset manager object so that it doesn't get garbage
    // collected by the Java VM.
    if (nativeAssetManager != NULL)
    {
        assetManagerRef = env->NewGlobalRef(assetManager);
    }

}


#ifdef __cplusplus
}
#endif
