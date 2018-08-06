LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(OCULUS_SDK_PATH)/cflags.mk

LIBVNCSERVER_PATH       := $(LOCAL_PATH)/../../thirdparty/libvncserver
LOCAL_MODULE            := ovrapp
LOCAL_SRC_FILES := \
	../../src/ovrapp.cpp \
	$(LIBVNCSERVER_PATH)/libvncclient/cursor.c \
	$(LIBVNCSERVER_PATH)/libvncclient/listen.c \
	$(LIBVNCSERVER_PATH)/libvncclient/rfbproto.c \
	$(LIBVNCSERVER_PATH)/libvncclient/sockets.c \
	$(LIBVNCSERVER_PATH)/libvncclient/vncviewer.c \
	$(LIBVNCSERVER_PATH)/common/minilzo.c \
	$(LIBVNCSERVER_PATH)/libvncclient/tls_none.c

LOCAL_STATIC_LIBRARIES := vrmodel vrappframework libovrkernel
LOCAL_SHARED_LIBRARIES := vrapi turbojpeg
LOCAL_CFLAGS += \
	-Wno-error \
	-isystem $(LIBVNCSERVER_PATH) \
	-isystem $(LIBVNCSERVER_PATH)/common \
	-isystem $(LOCAL_PATH)/../../include
LOCAL_CPPFLAGS += \
	-std=gnu++17 -pedantic -Wall -Wextra -fexceptions \
	-isystem $(LOCAL_PATH)/../../thirdparty/cpptoml/include

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := turbojpeg
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../thirdparty/libjpeg-turbo/$(TARGET_ARCH_ABI)/libturbojpeg.so
include $(PREBUILT_SHARED_LIBRARY)

$(call import-module,LibOVRKernel/Projects/Android/jni)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppFramework/Projects/Android/jni)
$(call import-module,VrAppSupport/VrModel/Projects/Android/jni)
