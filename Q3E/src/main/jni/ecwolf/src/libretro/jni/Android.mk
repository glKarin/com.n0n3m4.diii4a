LOCAL_PATH   := $(call my-dir)

HAVE_LZMA    := 1
CORE_DIR     := $(LOCAL_PATH)/../../..
LIBRETRO_DIR := $(LOCAL_PATH)/..

include $(LOCAL_PATH)/../Makefile.common

COREFLAGS := -DINLINE=inline -DHAVE_STDINT_H -DHAVE_INTTYPES_H -D__LIBRETRO__ -DVIDEO_RGB565 -DLIBRETRO -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DNO_STORE

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CXXFLAGS  := $(COREFLAGS) $(INCFLAGS)
LOCAL_CFLAGS    := $(INCFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
LOCAL_CPP_FEATURES += exceptions

LOCAL_C_INCLUDES = $(CORE_DIR) \
						 $(CORE_DIR)/src \
						 $(CORE_DIR)/src/libretro \
						 $(CORE_DIR)/src/g_shared

include $(BUILD_SHARED_LIBRARY)
