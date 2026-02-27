#ifndef MYFAKEAUTH_H_
#define MYFAKEAUTH_H_

extern int attack_fakeauth(JNIEnv* pEnv, jobject pThis, jmethodID pMid, char* interface_name, int reassoc_timing, char* essid, char* bssid, int use_custom_station_mac, char* station_mac, int keepalive_timing, int packet_timing, int id);
extern void stop_attack_fakeauth(int running);

#endif