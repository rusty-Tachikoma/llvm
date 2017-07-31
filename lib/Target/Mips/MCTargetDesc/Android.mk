LOCAL_PATH := $(call my-dir)

mips_mc_desc_TBLGEN_TABLES39 := \
  MipsGenRegisterInfo.inc \
  MipsGenInstrInfo.inc \
  MipsGenMCCodeEmitter.inc \
  MipsGenSubtargetInfo.inc

mips_mc_desc_SRC_FILES39 := \
  MipsABIFlagsSection.cpp \
  MipsABIInfo.cpp \
  MipsAsmBackend.cpp \
  MipsELFObjectWriter.cpp \
  MipsELFStreamer.cpp \
  MipsMCAsmInfo.cpp \
  MipsMCCodeEmitter.cpp \
  MipsMCExpr.cpp \
  MipsMCTargetDesc.cpp \
  MipsOptionRecord.cpp \
  MipsNaClELFStreamer.cpp \
  MipsTargetStreamer.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS39)

LOCAL_MODULE:= libLL39MipsDesc
LOCAL_MODULE_HOST_OS := darwin linux windows

LOCAL_SRC_FILES := $(mips_mc_desc_SRC_FILES39)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/..

TBLGEN_TD_DIR39 := $(LOCAL_PATH)/..
TBLGEN_TABLES39 := $(mips_mc_desc_TBLGEN_TABLES39)

include $(LLVM39_HOST_BUILD_MK)
include $(LLVM39_TBLGEN_RULES_MK)
include $(LLVM39_GEN_ATTRIBUTES_MK)
include $(LLVM39_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device only
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS39)

LOCAL_MODULE:= libLL39MipsDesc

LOCAL_SRC_FILES := $(mips_mc_desc_SRC_FILES39)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/..

TBLGEN_TD_DIR39 := $(LOCAL_PATH)/..
TBLGEN_TABLES39 := $(mips_mc_desc_TBLGEN_TABLES39)

include $(LLVM39_DEVICE_BUILD_MK)
include $(LLVM39_TBLGEN_RULES_MK)
include $(LLVM39_GEN_ATTRIBUTES_MK)
include $(LLVM39_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
