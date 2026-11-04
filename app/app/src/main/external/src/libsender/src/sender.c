#include <stddef.h>
#include "sender.h"

static JavaVM *g_jvm = NULL;
static jclass clazz = NULL;
static jmethodID mid = NULL;

void setup_send_env(JNIEnv *env) {
	jclass local_clazz;

	if (env == NULL)
		return;

	if ((*env)->GetJavaVM(env, &g_jvm) != JNI_OK)
		g_jvm = NULL;

	if (clazz != NULL) {
		(*env)->DeleteGlobalRef(env, clazz);
		clazz = NULL;
		mid = NULL;
	}

	local_clazz = (*env)->FindClass(env, "de/tu_darmstadt/seemoo/nexmon/net/FrameSender");
	if (local_clazz == NULL) {
		if ((*env)->ExceptionCheck(env))
			(*env)->ExceptionClear(env);
		return;
	}

	clazz = (jclass) (*env)->NewGlobalRef(env, local_clazz);
	(*env)->DeleteLocalRef(env, local_clazz);

	if (clazz != NULL) {
		mid = (*env)->GetStaticMethodID(env, clazz, "sendViaSocket", "([B)Z");
		if (mid == NULL && (*env)->ExceptionCheck(env))
			(*env)->ExceptionClear(env);
	}
}

void send_packet_to_java(char* packet, int len) {
	JNIEnv *env = NULL;
	jbyteArray jpacket;
	int attached = 0;
	jint get_env_res;

	if (g_jvm == NULL || clazz == NULL || mid == NULL || packet == NULL || len < 0)
		return;

	get_env_res = (*g_jvm)->GetEnv(g_jvm, (void **) &env, JNI_VERSION_1_6);
	if (get_env_res == JNI_EDETACHED) {
#ifdef __ANDROID__
		if ((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) != JNI_OK)
			return;
#else
		if ((*g_jvm)->AttachCurrentThread(g_jvm, (void **) &env, NULL) != JNI_OK)
			return;
#endif
		attached = 1;
	} else if (get_env_res != JNI_OK || env == NULL) {
		return;
	}

	jpacket = (*env)->NewByteArray(env, len);
	if (jpacket == NULL) {
		if ((*env)->ExceptionCheck(env))
			(*env)->ExceptionClear(env);
		if (attached)
			(*g_jvm)->DetachCurrentThread(g_jvm);
		return;
	}

	if (len > 0)
		(*env)->SetByteArrayRegion(env, jpacket, 0, len, (const jbyte *) packet);

	(*env)->CallStaticBooleanMethod(env, clazz, mid, jpacket);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	(*env)->DeleteLocalRef(env, jpacket);

	if (attached)
		(*g_jvm)->DetachCurrentThread(g_jvm);
}
