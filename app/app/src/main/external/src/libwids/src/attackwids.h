#ifndef ATTACKWIDS_H
#define ATTACKWIDS_H

extern int attack_wids(JNIEnv* pEnv, jobject pThis, jmethodID pMid, jmethodID pMid_stats, char* interface_name, char* ap_essid, int use_chan_hopping, int use_zero_chaos, int myid);
#endif