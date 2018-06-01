LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include ../../../../../cflags.mk

LIBVNCCLIENT_DIR        := $(LOCAL_PATH)/../../../libvncserver
LOCAL_MODULE			:= ovrapp
LOCAL_SRC_FILES			:= \
	../../../src/ovrapp.cpp \
    $(LIBVNCCLIENT_DIR)/libvncclient/cursor.c \
    $(LIBVNCCLIENT_DIR)/libvncclient/listen.c \
    $(LIBVNCCLIENT_DIR)/libvncclient/rfbproto.c \
    $(LIBVNCCLIENT_DIR)/libvncclient/sockets.c \
    $(LIBVNCCLIENT_DIR)/libvncclient/vncviewer.c \
    $(LIBVNCCLIENT_DIR)/common/minilzo.c \
    $(LIBVNCCLIENT_DIR)/libvncclient/tls_none.c

LOCAL_STATIC_LIBRARIES	:= vrsound vrmodel vrlocale vrgui vrappframework libovrkernel
LOCAL_SHARED_LIBRARIES	:= vrapi liblog
LOCAL_CPPFLAGS			+= -std=gnu++14 -pedantic -Wall -Wextra
LOCAL_CFLAGS			+= \
	-Wno-error \
	-I$(LOCAL_PATH)/../../../libvncserver \
	-I$(LOCAL_PATH)/../../../libvncserver/common \
	-I$(LOCAL_PATH)/../../../include

include $(BUILD_SHARED_LIBRARY)

$(call import-module,LibOVRKernel/Projects/Android/jni)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppFramework/Projects/Android/jni)
$(call import-module,VrAppSupport/VrGUI/Projects/Android/jni)
$(call import-module,VrAppSupport/VrLocale/Projects/Android/jni)
$(call import-module,VrAppSupport/VrModel/Projects/Android/jni)
$(call import-module,VrAppSupport/VrSound/Projects/Android/jni)
