/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2011 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_VERSION_H_
#define NETLINK_VERSION_H_

/* Compile Time Versioning Information */

#ifdef __cplusplus
extern "C" {
#endif

#define LIBNL_STRING "libnl 3.12.0"
#define LIBNL_VERSION "3.12.0"

#define LIBNL_VER_MAJ		3
#define LIBNL_VER_MIN		12
#define LIBNL_VER_MIC		0
#define LIBNL_VER(maj,min)	((maj) << 8 | (min))
#define LIBNL_VER_NUM		LIBNL_VER(LIBNL_VER_MAJ, LIBNL_VER_MIN)

#define LIBNL_CURRENT		226
#define LIBNL_REVISION		0
#define LIBNL_AGE		26

/* Run-time version information */

extern const int        nl_ver_num;
extern const int        nl_ver_maj;
extern const int        nl_ver_min;
extern const int        nl_ver_mic;

#ifdef __cplusplus
}
#endif

#endif
