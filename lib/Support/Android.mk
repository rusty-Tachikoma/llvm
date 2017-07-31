LOCAL_PATH:= $(call my-dir)
LLVM39_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM39_ROOT_PATH)/llvm.mk

support_SRC_FILES39 := $(sort $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)))
support_SRC_FILES39 += $(sort $(notdir $(wildcard $(LOCAL_PATH)/*.c)))

# For the host
# =====================================================
include $(CLEAR_VARS)

# FIXME: This only requires RTTI because tblgen uses it.  Fix that.
REQUIRES_RTTI := 1

LOCAL_SRC_FILES := $(support_SRC_FILES39)

LOCAL_MODULE:= libLL39Support

LOCAL_CFLAGS := -D__android__

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM39_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(support_SRC_FILES39)

LOCAL_MODULE:= libLL39Support

LOCAL_CFLAGS := -D__android__

include $(LLVM39_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)
endif
