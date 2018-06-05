# MAKEFILE_LIST specifies the current used Makefiles, of which this is the last
# one. I use that to obtain the Application.mk dir then import the root
# Application.mk.
ROOT_DIR := $(OCULUS_SDK_PATH)
include $(ROOT_DIR)/Application.mk

APP_CPPFLAGS += -frtti
APP_STL      := c++_static
