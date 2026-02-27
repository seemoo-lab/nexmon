#include <jni.h>
#include "infomessage.h"




void info_message_thread(char* mes, int reason, JNIEnv* env) {
    jstring jmes, jid;
	       
	jmes = (*env)->NewStringUTF(env, mes);

	(*env)->CallVoidMethod(env, cb_obj, cb_mid, jmes, (jint) reason, (jint) cb_id);

	(*env)->DeleteLocalRef(env, jmes);
    
}

void info_message(char* mes, int reason) {
    jstring jmes, jid;
	       
	jmes = (*cb_jnienv)->NewStringUTF(cb_jnienv, mes);

	(*cb_jnienv)->CallVoidMethod(cb_jnienv, cb_obj, cb_mid, jmes, (jint) reason, (jint) cb_id);

	(*cb_jnienv)->DeleteLocalRef(cb_jnienv, jmes);
    
}

void info_message_stats(int packet_rate, int packet_amount) {
	(*cb_jnienv)->CallVoidMethod(cb_jnienv, cb_obj, cb_mid_stats, (jint) packet_rate, (jint) packet_amount, (jint) cb_id);
}
void setup_info_message(JNIEnv* env, jobject obj, jmethodID mid, jmethodID mid_stats, int myid) {
	cb_jnienv = env;
	cb_obj = (*env)->NewGlobalRef(env, obj);
	cb_mid = mid;
	cb_mid_stats = mid_stats;
	cb_id = myid;
}

