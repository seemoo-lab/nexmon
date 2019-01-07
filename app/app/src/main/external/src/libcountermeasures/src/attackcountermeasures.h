#ifndef ATTACKCOUNTERMEASURES_H
#define ATTACKCOUNTERMEASURES_H

extern int attack_countermeasures(JNIEnv* pEnv, jobject pThis, jmethodID pMid, jmethodID pMid_stats, char* interface_name, char* ap_mac, int burst_pause, int packets_per_burst, int use_qos_exploit, int speed, int myid);

#endif