//
// Created by insswer on 2016/2/29.
//

#define LOG_TAG "BLACKSIREN_JNI"

#include <iostream>
#include <future>
#include <functional>
#include <thread>

#include "jni.h"
#include "JNIHelp.h"
#include <utils/misc.h>
#include <utils/Log.h>
#include <cutils/log.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <siren.h>
#include <lfqueue.h>

using namespace android;
using namespace BlackSiren;

static jclass blackSirenNativeClass;
static jclass blackSirenClass;
static jclass blackSirenVoicePackageClass;

//native callback
static jmethodID onInitInputStreamMethod;
static jmethodID onReleaseInputStreamMethod;
static jmethodID onStartInputStreamMethod;
static jmethodID onStopInputStreamMethod;
static jmethodID onReadInputStreamMethod;
static jmethodID onInputStreamErrorMethod;
static jmethodID onVoiceEventMethod;
static jmethodID onRawVoiceEventMethod;
static jmethodID onStateChangedMethod;

//global object
static jobject blackSirenNativeObject;
static jobject blackSirenObject;
static LFQueue *queue;
static std::thread dispatcherThread;
static void set_siren_handle(JNIEnv *env, jobject blackSirenNative, siren_t siren_handle) {
    jfieldID sirenFieldLocal = env->GetFieldID(blackSirenNativeClass, "_siren", "J");
    env->SetLongField(blackSirenNative, sirenFieldLocal, (jlong)siren_handle);
}

static siren_t get_siren_handle(JNIEnv *env, jobject blackSirenNative) {
    jfieldID sirenFieldLocal = env->GetFieldID(blackSirenNativeClass, "_siren", "J");
    jlong sirenLocal = env->GetLongField(blackSirenNative, sirenFieldLocal);
    return (siren_t)sirenLocal;
}

//siren object
static siren_input_if_t sirenInput;
static siren_proc_callback_t sirenProcCallback;
static siren_raw_stream_callback_t sirenRawStreamCallback;
static siren_state_changed_callback_t sirenStateChangedCallback;
static JavaVM *currentVM = nullptr;

template <typename T>
struct CallbackTask {
    CallbackTask(std::function<T> fn) : task(fn) {}
    std::function<T> task;
    std::promise<int> result_promise;
    void *data;
};

enum CallbackReturnCode {
    OK,
    FAILED,
    EXIT
};

static int javaAttachThread(const char *threadName, JNIEnv **env) {
    JavaVM *vm = currentVM;
    JavaVMAttachArgs args;
    jint result;
    if (vm == nullptr) {
        ALOGE("vm is null");
        *env = nullptr;
        return -1;
    }

    args.version = JNI_VERSION_1_6;
    args.name = (char *)threadName;
    args.group = nullptr;

    result = vm->AttachCurrentThread(env, (void *)&args);
    if (result != JNI_OK) {
        ALOGE("attach thread of %s failed", threadName);
        return -1;
    }

    return 0;
}

static int javaDetachThread() {
    JavaVM *vm = currentVM;

    if (vm == nullptr) {
        ALOGE("vm is null");
        return -1;
    }

    int result = vm->DetachCurrentThread();
    if (result != JNI_OK) {
        ALOGE("detach thread failed");
        return -1;
    }

    return 0;
}



static void callbackDispatchThread() {
    JNIEnv *env = nullptr;
    if (0 != javaAttachThread("callbackDispatcher", &env)) {
        ALOGI ("create callback dispatcher failed");
        return;
    }

    while (1) {
        CallbackTask<int(JNIEnv *, void *)> *task = nullptr;
        if (queue == nullptr) {
            ALOGE("exit with queue is nullptr");
            break;
        }

        if (queue->pop((void **)&task, nullptr) != 0) {
            ALOGI("callback dispatcher pop with error");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int result = task->task(env, (void *)task);
        task->result_promise.set_value(result);
        if (result == EXIT) {
            ALOGI("callback dispatcher thread exit");
            break;
        }
    }

    if (0 != javaDetachThread())
        ALOGE("detach callback dispatch failed");
}

static int init_input_stream(void *thiz) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return -1;
    }

    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        void *thiz_ = callback->data;
        if (onInitInputStreamMethod == nullptr || blackSirenObject == nullptr) {
            ALOGE("onInitInputStreamMethod is null or blackSirenObject is null");
            return FAILED;
        }       
        int ret = (int)env->CallIntMethod(blackSirenObject, onInitInputStreamMethod);
        return ret;
    });
    callbackTask->data = thiz;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    int ret = result.get();
    delete callbackTask;
    return ret;
}

static void release_input_stream(void *thiz) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        void *thiz_ = callback->data;
        if (onReleaseInputStreamMethod == nullptr || blackSirenObject == nullptr) {
            ALOGE("onReleaseInputStreamMethod is null or blackSirenObject is null");
            return FAILED;
        }

        env->CallVoidMethod(blackSirenObject, onReleaseInputStreamMethod);
        return OK;
    });
    callbackTask->data = thiz;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete callbackTask;
}

static void start_input_stream(void *thiz) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        void *thiz_ = callback->data;
        return 0;
    });
    callbackTask->data = thiz;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete callbackTask;
}

static void stop_input_stream(void *thiz) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        void *thiz_ = callback->data;
        return 0;
    });
    callbackTask->data = thiz;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete callbackTask;
}

static int read_input_stream(void *thiz, char *buff, int len) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return -1;
    }

    struct read_input_wrap {
        char *buff;
        void *thiz;
        int len;
    };
    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        return 0;
    });
    read_input_wrap *wrap = new read_input_wrap;
    wrap->len = len;
    wrap->thiz = thiz;
    wrap->buff = buff;
    callbackTask->data = (void *)wrap;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    int ret = result.get();
    delete wrap;
    delete callbackTask;
    return ret;
}

static void on_err_input_stream(void *thiz) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        void *thiz_ = callback->data;
        return 0;
    });
    callbackTask->data = thiz;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete callbackTask;
}

static void on_voice_event(void *thiz, int len, siren_event_t event,
                           void *buff, int has_sl, int has_voice, double sl_degree,
                           double energy, double threshold, int has_voiceprint) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    struct event_input_wrap {
        void *thiz;
        int len;
        siren_event_t event;
        void *buff;
        int has_sl;
        int has_voice;
        double sl_degree;
        double energy;
        double threshold;
        int has_voiceprint;
    };

    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        return 0;
    });
    event_input_wrap *wrap = new event_input_wrap;
    wrap->thiz = thiz;
    wrap->len = len;
    wrap->event = event;
    wrap->buff = buff;
    wrap->has_sl = has_sl;
    wrap->has_voice = has_voice;
    wrap->sl_degree = sl_degree;
    wrap->energy = energy;
    wrap->threshold = threshold;
    wrap->has_voiceprint = has_voiceprint;

    callbackTask->data = (void *)wrap;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete wrap;
    delete callbackTask;
}

static void on_raw_voice(void *thiz, int len, void *buff) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    struct raw_read_input_wrap {
        char *buff;
        void *thiz;
        int len;
    };
    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        return 0;
    });
    raw_read_input_wrap *wrap = new raw_read_input_wrap;
    wrap->len = len;
    wrap->thiz = thiz;
    wrap->buff = (char *)buff;
    callbackTask->data = (void *)wrap;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete wrap;
    delete callbackTask;
}

static void on_siren_state_changed(void *thiz, int current) {
    if (queue == nullptr) {
        ALOGE("read input with queue null");
        return;
    }

    struct state_change_wrap {
        void *thiz;
        int current;
    };
    CallbackTask<int(JNIEnv *, void *)> *callbackTask = new CallbackTask<int(JNIEnv*, void *)>(
    [](JNIEnv * env, void *thiz)->int {
        CallbackTask<int(JNIEnv *, void *)> *callback = (CallbackTask<int(JNIEnv *, void *)> *)thiz;
        return 0;
    });
    state_change_wrap *wrap = new state_change_wrap;
    wrap->current = current;
    wrap->thiz = thiz;
    callbackTask->data = (void *)wrap;
    std::future<int> result = callbackTask->result_promise.get_future();
    queue->push((void *)callbackTask);
    result.wait();
    delete wrap;
    delete callbackTask;
}

static void native_init_siren (JNIEnv *env, jobject thiz, jobject blackSirenObjectLocal, jobject pathStringObjectLocal) {
    //init queue and start thread
    queue = new LFQueue(256);
    std::thread t(callbackDispatchThread);
    dispatcherThread = std::move(t);


}

static void native_start_siren_process_stream(JNIEnv *env, jobject thiz) {

}

static void native_start_siren_raw_stream(JNIEnv *env, jobject thiz) {

}

static void native_stop_siren_process_stream(JNIEnv *env, jobject thiz) {

}

static void native_stop_siren_raw_stream(JNIEnv *env, jobject thiz) {

}

static void native_stop_siren_stream(JNIEnv *env, jobject thiz) {

}

static void native_set_siren_state(JNIEnv *env, jobject thiz, jint state) {

}

static void native_set_siren_steer(JNIEnv *env, jobject thiz, jfloat fo, jfloat ver) {

}

static void native_destroy_siren(JNIEnv *env, jobject thiz) {

}

static jint native_rebuild_vt_word_list(JNIEnv *env, jobject thiz, jobjectArray) {
    return 0;
}

static void nativeClassInit(JNIEnv *env) {
    jclass blackSirenNativeClassLocal = env->FindClass("com/rokid/openVoice/BlackSirenNative");
    blackSirenNativeClass = (jclass)env->NewGlobalRef((jobject)blackSirenNativeClassLocal);

    jclass blackSirenClassLocal = env->FindClass("com/rokid/openVoice/BlackSiren");
    blackSirenClass = (jclass)env->NewGlobalRef((jobject)blackSirenClassLocal);

    jclass blackSirenVoicePackageClassLocal = env->FindClass("com/rokid/openVoice/SirenVoicePackage");
    blackSirenVoicePackageClass = (jclass)env->NewGlobalRef((jobject)blackSirenVoicePackageClassLocal);

    jmethodID onInitInputStreamMethodLocal = env->GetMethodID(blackSirenClass, "onInitInputStreamNative", "()Z");
    onInitInputStreamMethod = (jmethodID)env->NewGlobalRef((jobject)onInitInputStreamMethodLocal);

    jmethodID onReleaseInputStreamMethodLocal = env->GetMethodID(blackSirenClass, "onReleaseInputStreamNative", "()V");
    onReleaseInputStreamMethod = (jmethodID)env->NewGlobalRef((jobject)onReleaseInputStreamMethodLocal);

    jmethodID onStartInputStreamMethodLocal = env->GetMethodID(blackSirenClass, "onStartInputStreamNative", "()V");
    onStartInputStreamMethod = (jmethodID)env->NewGlobalRef((jobject)onStartInputStreamMethodLocal);

    jmethodID onStopInputStreamMethodLocal = env->GetMethodID(blackSirenClass, "onStopInputStreamNative", "()V");
    onStopInputStreamMethod = (jmethodID)env->NewGlobalRef((jobject)onStopInputStreamMethodLocal);

    jmethodID onReadInputStreamMethodLocal = env->GetMethodID(blackSirenClass, "onReadInputStreamNative", "(Ljava/nio/ByteBuffer;I)I");
    onReadInputStreamMethod = (jmethodID)env->NewGlobalRef((jobject)onReadInputStreamMethodLocal);

    jmethodID onInputStreamErrorMethodLocal = env->GetMethodID(blackSirenClass, "onInputStreamErrorNative", "()V");
    onInputStreamErrorMethod = (jmethodID)env->NewGlobalRef((jobject)onInputStreamErrorMethodLocal);

    jmethodID onVoiceEventMethodLocal = env->GetMethodID(blackSirenClass, "onVoiceEventNative", "(Lcom/rokid/openVoice/SirenVoicePackage;)V");
    onVoiceEventMethod = (jmethodID)env->NewGlobalRef((jobject)onVoiceEventMethodLocal);

    jmethodID onRawVoiceEventMethodLocal = env->GetMethodID(blackSirenClass, "onRawVoiceEventNative", "(Ljava/nio/ByteBuffer;I)V");
    onRawVoiceEventMethod = (jmethodID)env->NewGlobalRef((jobject)onRawVoiceEventMethodLocal);

    jmethodID onStateChangedMethodLocal = env->GetMethodID(blackSirenClass, "onStateChangedNative", "(I)V");
    onStateChangedMethod = (jmethodID)env->NewGlobalRef((jobject)onStateChangedMethodLocal);
}

static void callbackInit(JNIEnv *env) {
    sirenInput.init_input = init_input_stream;
    sirenInput.release_input = release_input_stream;
    sirenInput.start_input = start_input_stream;
    sirenInput.stop_input = stop_input_stream;
    sirenInput.read_input = read_input_stream;
    sirenInput.on_err_input = on_err_input_stream;

    sirenProcCallback.voice_event_callback = on_voice_event;
    sirenRawStreamCallback.raw_voice_callback = on_raw_voice;
    sirenStateChangedCallback.state_changed_callback = on_siren_state_changed;
}

static const JNINativeMethod method_table[] = {
    {"init_siren_native", "(Lcom/rokid/openVoice/BlackSiren;Ljava/lang/String;)I", (void *) native_init_siren},
    {"start_siren_process_stream_native", "()V", (void *) native_start_siren_process_stream},
    {"start_siren_raw_stream_native", "()V", (void *) native_start_siren_raw_stream},
    {"stop_siren_process_stream_native", "()V", (void *) native_stop_siren_process_stream},
    {"stop_siren_raw_stream_native", "()V", (void *) native_stop_siren_raw_stream},
    {"stop_siren_stream_native", "()V", (void *) native_stop_siren_stream},
    {"set_siren_state_native", "(I)V", (void *) native_set_siren_state},
    {"set_siren_steer_native", "(FF)V", (void *) native_set_siren_steer},
    {"destroy_siren_native", "()V", (void *) native_destroy_siren},
    {"rebuild_vt_word_list_native", "([java/lang/String;])I", (void *) native_rebuild_vt_word_list}
};

int register_black_siren (JNIEnv *env) {
    const char *kclass = "com/rokid/openVoice/BlackSirenNative";
    return jniRegisterNativeMethods (env, kclass, method_table, NELEM(method_table));
}

/*
 * JNI Initialization
 */
jint JNI_OnLoad(JavaVM *jvm, void *reserved) {
    JNIEnv *e;
    int status;

    currentVM = jvm;
    ALOGV("BlackSiren: loading JNI\n");
    // Check JNI version
    if (jvm->GetEnv((void **)&e, JNI_VERSION_1_6)) {
        ALOGE("JNI version mismatch error");
        return JNI_ERR;
    }

    if ((status = register_black_siren(e)) < 0) {
        ALOGE("jni adapter service registration failure, status: %d", status);
        return JNI_ERR;
    }

    //init native class
    nativeClassInit(e);
    callbackInit(e);

    return JNI_VERSION_1_6;
}
