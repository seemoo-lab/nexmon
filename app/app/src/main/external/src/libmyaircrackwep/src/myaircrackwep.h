#ifndef MYAIRCRACKWEP_H_
#define MYAIRCRECKWEP_H_
   
extern void stop_attack_crack_wep(int stop_attack);

extern int attack_crack_wep(JNIEnv* pEnv, jobject pThis, jmethodID pMid, char* ssid, int use_essid, char* essid, char* file, int use_decloak, int use_korek, int id);


#endif