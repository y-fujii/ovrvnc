# MAKEFILE_LIST specifies the current used Makefiles, of which this is the last
# one. I use that to obtain the Application.mk dir then import the root
# Application.mk.
include $(OCULUS_SDK_PATH)/Application.mk

APP_STL := c++_static
