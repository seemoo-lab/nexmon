#ifndef SENDER_H_
#define SENDER_H_

#include <jni.h>



extern void setup_send_env(JNIEnv *env);

extern void send_packet_to_java(char* packet, int len);

#endif