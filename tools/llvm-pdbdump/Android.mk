LOCAL_PATH := $(call my-dir)

LLVM39_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM39_ROOT_PATH)/llvm.mk

llvm_pdbdump_SRC_FILES39 := $(sort $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)))

llvm_pdbdump_STATIC_LIBRARIES := \
  libLLVM39DebugInfoPDB \
  libLLVM39DebugInfoCodeView \
  libLLVM39Support

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-pdbdump
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(llvm_pdbdump_SRC_FILES39)

LOCAL_STATIC_LIBRARIES := $(llvm_pdbdump_STATIC_LIBRARIES)

LOCAL_LDLIBS += -lpthread -lm -ldl

include $(LLVM39_HOST_BUILD_MK)
include $(LLVM39_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
