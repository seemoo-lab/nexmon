#include "sender.h"

JNIEnv *g_env;

jclass clazz;
jmethodID mid;
jbyteArray jpacket;

void setup_send_env(JNIEnv *env) {
	g_env = env;
	clazz = (*g_env)->FindClass(g_env, "de/tu_darmstadt/seemoo/nexmon/net/FrameSender");
	
	if(clazz != 0)
		mid = (*g_env)->GetStaticMethodID(g_env, clazz, "sendViaSocket", "([B)Z");
}

void send_packet_to_java(char* packet, int len) {
	
	jpacket = (*g_env)->NewByteArray(g_env, len);

	(*g_env)->SetByteArrayRegion(g_env, jpacket, 0, len, (jbyte*)packet);

	if(clazz != 0 && mid != 0) {
		(*g_env)->CallStaticBooleanMethod(g_env, clazz, mid, jpacket);
	}
	
	(*g_env)->DeleteLocalRef(g_env, jpacket);
}