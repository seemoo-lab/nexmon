/*
 * mysniffer.h
 *
 *  Created on: Jun 2, 2016
 *      Author: fabian
 */

#ifndef MYSNIFFER_H_
#define MYSNIFFER_H_



extern char * sniff_it(JNIEnv *env, jobject pThis, jmethodID mid);
extern void stop_sniffer();

#endif