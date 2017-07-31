LOCAL_PATH := $(call my-dir)
LLVM39_ROOT_PATH := $(LOCAL_PATH)
LLVM39_TBLGEN := out/host/linux-x86/bin/ll39-tblgen
FORCE_BUILD_LLVM_DISABLE_NDEBUG ?= false
# Legality check: FORCE_BUILD_LLVM_DISABLE_NDEBUG should consist of one word -- either "true" or "false".
ifneq "$(words $(FORCE_BUILD_LLVM_DISABLE_NDEBUG))$(words $(filter-out true false,$(FORCE_BUILD_LLVM_DISABLE_NDEBUG)))" "10"
  $(error FORCE_BUILD_LLVM_DISABLE_NDEBUG may only be true, false, or unset)
endif

FORCE_BUILD_LLVM_DEBUG ?= false
# Legality check: FORCE_BUILD_LLVM_DEBUG should consist of one word -- either "true" or "false".
ifneq "$(words $(FORCE_BUILD_LLVM_DEBUG))$(words $(filter-out true false,$(FORCE_BUILD_LLVM_DEBUG)))" "10"
  $(error FORCE_BUILD_LLVM_DEBUG may only be true, false, or unset)
endif

include $(CLEAR_VARS)

# LLVM Libraries
subdirs := \
  lib/Analysis \
  lib/AsmParser \
  lib/Bitcode/Reader \
  lib/Bitcode/Writer \
  lib/ExecutionEngine \
  lib/ExecutionEngine/RuntimeDyld \
  lib/ExecutionEngine/MCJIT \
  lib/ExecutionEngine/Orc \
  lib/ExecutionEngine/Interpreter \
  lib/Fuzzer \
  lib/CodeGen \
  lib/CodeGen/AsmPrinter \
  lib/CodeGen/GlobalISel \
  lib/CodeGen/MIRParser \
  lib/CodeGen/SelectionDAG \
  lib/DebugInfo/CodeView \
  lib/DebugInfo/DWARF \
  lib/DebugInfo/PDB \
  lib/DebugInfo/Symbolize \
  lib/IR \
  lib/IRReader \
  lib/LibDriver \
  lib/Linker \
  lib/LTO \
  lib/MC \
  lib/MC/MCDisassembler \
  lib/MC/MCParser \
  lib/Object \
  lib/ObjectYAML \
  lib/Option \
  lib/Passes \
  lib/ProfileData \
  lib/ProfileData/Coverage \
  lib/Support \
  lib/TableGen \
  lib/Target \
  lib/Transforms/Hello \
  lib/Transforms/IPO \
  lib/Transforms/InstCombine \
  lib/Transforms/Instrumentation \
  lib/Transforms/ObjCARC \
  lib/Transforms/Scalar \
  lib/Transforms/Utils \
  lib/Transforms/Vectorize \

# ARM Code Generation Libraries
subdirs += \
  lib/Target/ARM \
  lib/Target/ARM/AsmParser \
  lib/Target/ARM/InstPrinter \
  lib/Target/ARM/Disassembler \
  lib/Target/ARM/MCTargetDesc \
  lib/Target/ARM/TargetInfo

# AArch64 Code Generation Libraries
subdirs += \
  lib/Target/AArch64  \
  lib/Target/AArch64/AsmParser \
  lib/Target/AArch64/InstPrinter \
  lib/Target/AArch64/Disassembler \
  lib/Target/AArch64/MCTargetDesc \
  lib/Target/AArch64/TargetInfo \
  lib/Target/AArch64/Utils

# MIPS Code Generation Libraries
subdirs += \
  lib/Target/Mips \
  lib/Target/Mips/AsmParser \
  lib/Target/Mips/InstPrinter \
  lib/Target/Mips/Disassembler \
  lib/Target/Mips/MCTargetDesc \
  lib/Target/Mips/TargetInfo

# X86 Code Generation Libraries
subdirs += \
  lib/Target/X86 \
  lib/Target/X86/AsmParser \
  lib/Target/X86/InstPrinter \
  lib/Target/X86/Disassembler \
  lib/Target/X86/MCTargetDesc \
  lib/Target/X86/TargetInfo \
  lib/Target/X86/Utils

ifneq ($(filter radeonsi,$(BOARD_GPU_DRIVERS)),)
# AMDGPU Code Generation Libraries
subdirs += \
  lib/Target/AMDGPU \
  lib/Target/AMDGPU/AsmParser \
  lib/Target/AMDGPU/InstPrinter \
  lib/Target/AMDGPU/MCTargetDesc \
  lib/Target/AMDGPU/TargetInfo \
  lib/Target/AMDGPU/Utils
endif

# LLVM Command Line Utilities
subdirs += \
  utils/TableGen \

include $(LOCAL_PATH)/llvm.mk
include $(LOCAL_PATH)/shared_llvm.mk

include $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, $(subdirs)))
