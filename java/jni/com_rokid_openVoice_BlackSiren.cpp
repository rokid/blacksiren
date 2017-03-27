//
// Created by insswer on 2016/2/29.
//

#define LOG_TAG "RKSirenNative_JNI"

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <utils/misc.h>
#include <utils/Log.h>
#include <cutils/log.h>
#include <cutils/sched_policy.h>
#include <hardware/hardware.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define _GUN_SOURCE
#include <sched.h>
#undef _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <pthread.h>

#include <siren.h>
using namespace android;

static jlong native_siren_openSirenProxy (JNIEnv *env, jobject thiz) {
	SirenProxy *siren = new SirenProxy;
	//connect to server
	siren->init();
	//get buffer, buffer will saved in proxy
	siren->retrieveAlgorithmBuffer();
	return (jlong)siren;
}

static jobject native_siren_readVoicePool (JNIEnv *env, jobject thiz, jlong proxy, jint token_, jint block) {
	if (proxy == 0) {
		ALOGE ("proxy is null");
		return NULL;
	}

	SirenProxy *siren = (SirenProxy *)proxy;
	int size = 0;
	char *buff = NULL;
	float energy = 0.0f;
	float threshold = 0.0f;

	if (0 != readAlgorithmStream(siren, token_, block, &size, &buff, &energy, &threshold)) {
		ALOGE ("readAlgorithmStream failed!");
		return NULL;
	}

	jbyteArray jBuff = env->NewByteArray(size);
	env->SetByteArrayRegion (jBuff, 0, size, (jbyte *)buff);
	jclass package_class = env->FindClass("rokid/os/SirenVoicePackage");
	jmethodID construct_method = env->GetMethodID (package_class,
													"<init>", "(IFF[B)V");
	jobject package_obj = env->NewObject (package_class, construct_method,
														size, energy, threshold, jBuff);

	free (buff);
	return package_obj;
}

static const JNINativeMethod method_table[] = {
	{"native_siren_openSirenProxy", "()J", (void *) native_siren_openSirenProxy},
	{"native_siren_readVoicePool", "(JII)Lrokid/os/SirenVoicePackage;", (void *) native_siren_readVoicePool},
	{"native_siren_boostPriority", "(II)V", (void *) native_siren_boostPriority},
};

int register_black_siren (JNIEnv *env)
{
	const char *kclass = "com/rokid/openVoice/BlackSirenNative";
	return jniRegisterNativeMethods (env, kclass, method_table, NELEM(method_table));
}

/*
 * JNI Initialization
 */
jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv *e;
    int status;

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

	return JNI_VERSION_1_6;
}
