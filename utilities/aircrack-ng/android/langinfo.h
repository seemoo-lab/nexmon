/* bionic has no <langinfo.h>; aircrack-ng only needs CODESET. */
#ifndef AIRCRACK_NG_ANDROID_LANGINFO_H
#define AIRCRACK_NG_ANDROID_LANGINFO_H

#define CODESET 0

static inline char * nl_langinfo(int item)
{
	(void) item;
	return "UTF-8";
}

#endif
