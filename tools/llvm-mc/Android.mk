LOCAL_PATH := $(call my-dir)

LLVM39_ROOT_PATH := $(LOCAL_PATH)/../..


#===---------------------------------------------------------------===
# llvm-mc command line tool
#===---------------------------------------------------------------===

llvm_mc_SRC_FILES39 := \
  llvm-mc.cpp \
  Disassembler.cpp

llvm_mc_STATIC_LIBRARIES := \
  libLL39ARMAsmParser \
  libLL39ARMInfo \
  libLL39ARMDesc \
  libLL39ARMAsmPrinter \
  libLL39ARMDisassembler \
  libLL39AArch64Info \
  libLL39AArch64AsmParser \
  libLL39AArch64Desc \
  libLL39AArch64AsmPrinter \
  libLL39AArch64Utils \
  libLL39AArch64Disassembler \
  libLL39MipsInfo \
  libLL39MipsAsmParser \
  libLL39MipsDesc \
  libLL39MipsAsmPrinter \
  libLL39MipsDisassembler \
  libLL39X86Info \
  libLL39X86Desc \
  libLL39X86AsmParser \
  libLL39X86AsmPrinter \
  libLL39X86Utils \
  libLL39X86Disassembler \
  libLL39X86CodeGen \
  libLL39AsmPrinter \
  libLL39CodeGen \
  libLL39TransformUtils \
  libLL39Target \
  libLL39Analysis \
  libLL39MC \
  libLL39Object \
  libLL39BitReader \
  libLL39MCParser \
  libLL39Core \
  libLL39AsmParser \
  libLL39Support \
  libLL39MCDisassembler \

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-mc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(llvm_mc_SRC_FILES39)

LOCAL_STATIC_LIBRARIES := $(llvm_mc_STATIC_LIBRARIES)

LOCAL_LDLIBS += -lpthread -lm -ldl

include $(LLVM39_ROOT_PATH)/llvm.mk
include $(LLVM39_HOST_BUILD_MK)
include $(LLVM39_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
