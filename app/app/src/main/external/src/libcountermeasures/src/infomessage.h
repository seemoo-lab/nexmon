#ifndef INFOMESSAGE_H_
#define INFOMESSAGE_H_

#include <jni.h>

#define UPDATE_ERROR 1
#define UPDATE_SUCCESS 2
#define UPDATE_RUNNING 3

JNIEnv *cb_jnienv;
jobject cb_obj;
jmethodID cb_mid;
jmethodID cb_mid_stats;

int cb_id;
JavaVM* jvm;

void info_message_thread(char* mes, int reason, JNIEnv* env);

void info_message(char* mes, int reason);

void info_message_stats(int packet_rate, int packet_amount);

void setup_info_message(JNIEnv* env, jobject obj, jmethodID mid, jmethodID mid_stats, int myid);

#endif