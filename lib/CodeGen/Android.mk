LOCAL_PATH:= $(call my-dir)

codegen_SRC_FILES := \
  AggressiveAntiDepBreaker.cpp \
  AllocationOrder.cpp \
  Analysis.cpp \
  AtomicExpandPass.cpp \
  BasicTargetTransformInfo.cpp \
  BranchCoalescing.cpp \
  BranchFolding.cpp \
  BranchRelaxation.cpp \
  BuiltinGCs.cpp \
  CalcSpillWeights.cpp \
  CallingConvLower.cpp \
  CodeGen.cpp \
  CodeGenPrepare.cpp \
  CountingFunctionInserter.cpp \
  CriticalAntiDepBreaker.cpp \
  DeadMachineInstructionElim.cpp \
  DetectDeadLanes.cpp \
  DFAPacketizer.cpp \
  DwarfEHPrepare.cpp \
  EarlyIfConversion.cpp \
  EdgeBundles.cpp \
  ExecutionDepsFix.cpp \
  ExpandISelPseudos.cpp \
  ExpandPostRAPseudos.cpp \
  ExpandReductions.cpp \
  FaultMaps.cpp \
  FEntryInserter.cpp \
  FuncletLayout.cpp \
  GCMetadata.cpp \
  GCMetadataPrinter.cpp \
  GCRootLowering.cpp \
  GCStrategy.cpp \
  GlobalMerge.cpp \
  IfConversion.cpp \
  ImplicitNullChecks.cpp \
  InlineSpiller.cpp \
  InterferenceCache.cpp \
  InterleavedAccessPass.cpp \
  IntrinsicLowering.cpp \
  LatencyPriorityQueue.cpp \
  LazyMachineBlockFrequencyInfo.cpp \
  LexicalScopes.cpp \
  LiveDebugValues.cpp \
  LiveDebugVariables.cpp \
  LiveIntervalAnalysis.cpp \
  LiveInterval.cpp \
  LiveIntervalUnion.cpp \
  LivePhysRegs.cpp \
  LiveRangeCalc.cpp \
  LiveRangeEdit.cpp \
  LiveRangeShrink.cpp \
  LiveRegMatrix.cpp \
  LiveRegUnits.cpp \
  LiveStackAnalysis.cpp \
  LiveVariables.cpp \
  LLVMTargetMachine.cpp \
  LocalStackSlotAllocation.cpp \
  LowLevelType.cpp \
  LowerEmuTLS.cpp \
  MachineBasicBlock.cpp \
  MachineBlockFrequencyInfo.cpp \
  MachineBlockPlacement.cpp \
  MachineBranchProbabilityInfo.cpp \
  MachineCombiner.cpp \
  MachineCopyPropagation.cpp \
  MachineCSE.cpp \
  MachineDominanceFrontier.cpp \
  MachineDominators.cpp \
  MachineFrameInfo.cpp \
  MachineFunction.cpp \
  MachineFunctionPass.cpp \
  MachineFunctionPrinterPass.cpp \
  MachineInstrBundle.cpp \
  MachineInstr.cpp \
  MachineLICM.cpp \
  MachineLoopInfo.cpp \
  MachineModuleInfo.cpp \
  MachineModuleInfoImpls.cpp \
  MachineOptimizationRemarkEmitter.cpp \
  MachineOutliner.cpp \
  MachinePassRegistry.cpp \
  MachinePipeliner.cpp \
  MachinePostDominators.cpp \
  MachineRegionInfo.cpp \
  MachineRegisterInfo.cpp \
  MachineScheduler.cpp \
  MachineSink.cpp \
  MachineSSAUpdater.cpp \
  MachineTraceMetrics.cpp \
  MachineVerifier.cpp \
  PatchableFunction.cpp \
  MIRPrinter.cpp \
  MIRPrintingPass.cpp \
  MacroFusion.cpp \
  OptimizePHIs.cpp \
  ParallelCG.cpp \
  PeepholeOptimizer.cpp \
  PHIElimination.cpp \
  PHIEliminationUtils.cpp \
  PostRAHazardRecognizer.cpp \
  PostRASchedulerList.cpp \
  PreISelIntrinsicLowering.cpp \
  ProcessImplicitDefs.cpp \
  PrologEpilogInserter.cpp \
  PseudoSourceValue.cpp \
  RegAllocBase.cpp \
  RegAllocBasic.cpp \
  RegAllocFast.cpp \
  RegAllocGreedy.cpp \
  RegAllocPBQP.cpp \
  RegisterClassInfo.cpp \
  RegisterCoalescer.cpp \
  RegisterPressure.cpp \
  RegisterScavenging.cpp \
  RenameIndependentSubregs.cpp \
  RegisterUsageInfo.cpp \
  RegUsageInfoCollector.cpp \
  RegUsageInfoPropagate.cpp \
  ResetMachineFunctionPass.cpp \
  SafeStack.cpp \
  SafeStackColoring.cpp \
  SafeStackLayout.cpp \
  ScalarizeMaskedMemIntrin.cpp \
  ScheduleDAG.cpp \
  ScheduleDAGInstrs.cpp \
  ScheduleDAGPrinter.cpp \
  ScoreboardHazardRecognizer.cpp \
  ShadowStackGCLowering.cpp \
  ShrinkWrap.cpp \
  SjLjEHPrepare.cpp \
  SlotIndexes.cpp \
  SpillPlacement.cpp \
  SplitKit.cpp \
  StackColoring.cpp \
  StackMapLivenessAnalysis.cpp \
  StackMaps.cpp \
  StackProtector.cpp \
  StackSlotColoring.cpp \
  TailDuplication.cpp \
  TailDuplicator.cpp \
  TargetFrameLoweringImpl.cpp \
  TargetInstrInfo.cpp \
  TargetLoweringBase.cpp \
  TargetLoweringObjectFileImpl.cpp \
  TargetOptionsImpl.cpp \
  TargetPassConfig.cpp \
  TargetRegisterInfo.cpp \
  TargetSchedule.cpp \
  TargetSubtargetInfo.cpp \
  TwoAddressInstructionPass.cpp \
  UnreachableBlockElim.cpp \
  VirtRegMap.cpp \
  WinEHPrepare.cpp \
  XRayInstrumentation.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(codegen_SRC_FILES)
LOCAL_MODULE:= libLLVM60CodeGen

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM60_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(codegen_SRC_FILES)
LOCAL_MODULE:= libLLVM60CodeGen

include $(LLVM60_DEVICE_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
