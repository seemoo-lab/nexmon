#ifndef _JNIEXCEPT_H_
#define _JNIEXCEPT_H_

#include <jni.h>

void throwDissectorException(char *file, unsigned int lineno, char *expression);
void jniExceptionInit(JNIEnv *env);

#endif
