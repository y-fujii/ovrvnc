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

LOCAL_STATIC_LIBRARIES	:= vrmodel vrappframework libovrkernel
LOCAL_SHARED_LIBRARIES	:= vrapi liblog
LOCAL_CFLAGS += \
	-Wno-error \
	-isystem $(LOCAL_PATH)/../../../libvncserver \
	-isystem $(LOCAL_PATH)/../../../libvncserver/common \
	-isystem $(LOCAL_PATH)/../../../include
LOCAL_CPPFLAGS += \
	-std=gnu++14 -pedantic -Wall -Wextra -fexceptions -frtti \
	-isystem $(LOCAL_PATH)/../../../cpptoml/include \

include $(BUILD_SHARED_LIBRARY)

$(call import-module,LibOVRKernel/Projects/Android/jni)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppFramework/Projects/Android/jni)
$(call import-module,VrAppSupport/VrGUI/Projects/Android/jni)
$(call import-module,VrAppSupport/VrLocale/Projects/Android/jni)
$(call import-module,VrAppSupport/VrModel/Projects/Android/jni)
$(call import-module,VrAppSupport/VrSound/Projects/Android/jni)
