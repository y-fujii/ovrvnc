LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(OCULUS_SDK_PATH)/cflags.mk

TIGERVNC_PATH   := $(LOCAL_PATH)/../../thirdparty/tigervnc/common
LOCAL_MODULE    := ovrapp
LOCAL_SRC_FILES := \
	../../src/ovrapp.cpp \
	$(TIGERVNC_PATH)/os/Thread.cxx \
	$(TIGERVNC_PATH)/os/Mutex.cxx \
	$(TIGERVNC_PATH)/Xregion/Region.c \
	$(TIGERVNC_PATH)/network/Socket.cxx \
	$(TIGERVNC_PATH)/network/TcpSocket.cxx \
	$(TIGERVNC_PATH)/rdr/Exception.cxx \
	$(TIGERVNC_PATH)/rdr/FdInStream.cxx \
	$(TIGERVNC_PATH)/rdr/FdOutStream.cxx \
	$(TIGERVNC_PATH)/rdr/HexInStream.cxx \
	$(TIGERVNC_PATH)/rdr/HexOutStream.cxx \
	$(TIGERVNC_PATH)/rdr/InStream.cxx \
	$(TIGERVNC_PATH)/rdr/ZlibInStream.cxx \
	$(TIGERVNC_PATH)/rdr/ZlibOutStream.cxx \
	$(TIGERVNC_PATH)/rfb/d3des.c \
	$(TIGERVNC_PATH)/rfb/CConnection.cxx \
	$(TIGERVNC_PATH)/rfb/CMsgHandler.cxx \
	$(TIGERVNC_PATH)/rfb/CMsgReader.cxx \
	$(TIGERVNC_PATH)/rfb/CMsgWriter.cxx \
	$(TIGERVNC_PATH)/rfb/CSecurityPlain.cxx \
	$(TIGERVNC_PATH)/rfb/CSecurityStack.cxx \
	$(TIGERVNC_PATH)/rfb/CSecurityVeNCrypt.cxx \
	$(TIGERVNC_PATH)/rfb/CSecurityVncAuth.cxx \
	$(TIGERVNC_PATH)/rfb/Configuration.cxx \
	$(TIGERVNC_PATH)/rfb/ConnParams.cxx \
	$(TIGERVNC_PATH)/rfb/CopyRectDecoder.cxx \
	$(TIGERVNC_PATH)/rfb/Cursor.cxx \
	$(TIGERVNC_PATH)/rfb/DecodeManager.cxx \
	$(TIGERVNC_PATH)/rfb/Decoder.cxx \
	$(TIGERVNC_PATH)/rfb/HextileDecoder.cxx \
	$(TIGERVNC_PATH)/rfb/JpegDecompressor.cxx \
	$(TIGERVNC_PATH)/rfb/LogWriter.cxx \
	$(TIGERVNC_PATH)/rfb/Logger.cxx \
	$(TIGERVNC_PATH)/rfb/Password.cxx \
	$(TIGERVNC_PATH)/rfb/PixelBuffer.cxx \
	$(TIGERVNC_PATH)/rfb/PixelFormat.cxx \
	$(TIGERVNC_PATH)/rfb/RREDecoder.cxx \
	$(TIGERVNC_PATH)/rfb/RawDecoder.cxx \
	$(TIGERVNC_PATH)/rfb/Region.cxx \
	$(TIGERVNC_PATH)/rfb/Security.cxx \
	$(TIGERVNC_PATH)/rfb/SecurityClient.cxx \
	$(TIGERVNC_PATH)/rfb/TightDecoder.cxx \
	$(TIGERVNC_PATH)/rfb/ZRLEDecoder.cxx \
	$(TIGERVNC_PATH)/rfb/util.cxx

LOCAL_STATIC_LIBRARIES := vrmodel vrappframework libovrkernel
LOCAL_SHARED_LIBRARIES := vrapi jpeg
LOCAL_CPPFLAGS += \
	-std=gnu++17 -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-vla-extension -fexceptions \
	-isystem $(TIGERVNC_PATH) \
	-isystem $(LOCAL_PATH)/../../thirdparty/cpptoml/include \
	-isystem $(LOCAL_PATH)/../../thirdparty/libjpeg-turbo/$(TARGET_ARCH_ABI)/usr/include

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := jpeg
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../thirdparty/libjpeg-turbo/$(TARGET_ARCH_ABI)/usr/lib/libjpeg.so
include $(PREBUILT_SHARED_LIBRARY)

$(call import-module,LibOVRKernel/Projects/Android/jni)
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrAppFramework/Projects/Android/jni)
$(call import-module,VrAppSupport/VrModel/Projects/Android/jni)
