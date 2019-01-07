LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := myandroidlogger
LOCAL_SRC_FILES := ../../libs/armeabi/libandroidlogger.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := myairdecap-ng
LOCAL_SRC_FILES := ../../libs/armeabi/libmyairdecap-ng.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := myfakeauth
LOCAL_SRC_FILES := ../../libs/armeabi/libmyfakeauth.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := myaircrackwep
LOCAL_SRC_FILES := ../../libs/armeabi/libmyaircrackwep.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mylibpcap
LOCAL_SRC_FILES := ../../libs/armeabi/libpcap.a
LOCAL_SHARED_LIBRARIES:=myandroidlogger
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := authflood
LOCAL_SRC_FILES := ../../libs/armeabi/libauthflood.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := beaconflood
LOCAL_SRC_FILES := ../../libs/armeabi/libbeaconflood.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := countermeasures
LOCAL_SRC_FILES := ../../libs/armeabi/libcountermeasures.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := wids
LOCAL_SRC_FILES := ../../libs/armeabi/libwids.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ivstools
LOCAL_SRC_FILES := ../../libs/armeabi/libmyivstools.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := upchack
LOCAL_SRC_FILES := ../../libs/armeabi/libmyupchack.so
include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := reaver
#LOCAL_SRC_FILES := ../../libs/armeabi/libreaver.so
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := cowpatty
LOCAL_SRC_FILES := ../../libs/armeabi/libcowpatty.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE:= nexmonpentestsuite
LOCAL_SRC_FILES := wrapper_pentestsuite.c
LOCAL_STATIC_LIBRARIES:=mylibpcap
LOCAL_SHARED_LIBRARIES:=myairdecap-ng myfakeauth myaircrackwep authflood beaconflood countermeasures wids ivstools upchack cowpatty
include $(BUILD_SHARED_LIBRARY)


