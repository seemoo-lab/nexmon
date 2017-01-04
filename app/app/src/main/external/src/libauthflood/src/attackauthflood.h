#ifndef ATTACKAUTHFLOOD_H
#define ATTACKAUTHFLOOD_H

extern int attack_auth_flood(JNIEnv* pEnv, jobject pThis, jmethodID pMid, jmethodID pMid_stats, char* interface_name, char* ap_mac, int intelligent, int myid);
#endif