LOCAL_PATH:= $(call my-dir)

llvm_as_SRC_FILES39 := \
  llvm-as.cpp

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-as
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(llvm_as_SRC_FILES39)
LOCAL_LDLIBS += -lm
LOCAL_LDLIBS_windows := -limagehlp -lversion
LOCAL_LDLIBS_darwin := -lpthread -ldl
LOCAL_LDLIBS_linux := -lpthread -ldl

LOCAL_STATIC_LIBRARIES := \
  libLLVM39AsmParser \
  libLLVM39BitWriter \
  libLLVM39Analysis \
  libLLVM39Core \
  libLLVM39Support

include $(LLVM39_HOST_BUILD_MK)
include $(LLVM39_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_EXECUTABLE)
