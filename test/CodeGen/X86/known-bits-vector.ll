; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+avx | FileCheck %s --check-prefix=X32
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx | FileCheck %s --check-prefix=X64

define i32 @knownbits_mask_extract_sext(<8 x i16> %a0) nounwind {
; X32-LABEL: knownbits_mask_extract_sext:
; X32:       # BB#0:
; X32-NEXT:    vpand {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpextrw $0, %xmm0, %eax
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_extract_sext:
; X64:       # BB#0:
; X64-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpextrw $0, %xmm0, %eax
; X64-NEXT:    retq
  %1 = and <8 x i16> %a0, <i16 15, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>
  %2 = extractelement <8 x i16> %1, i32 0
  %3 = sext i16 %2 to i32
  ret i32 %3
}

define float @knownbits_mask_extract_uitofp(<2 x i64> %a0) nounwind {
; X32-LABEL: knownbits_mask_extract_uitofp:
; X32:       # BB#0:
; X32-NEXT:    pushl %ebp
; X32-NEXT:    movl %esp, %ebp
; X32-NEXT:    andl $-8, %esp
; X32-NEXT:    subl $16, %esp
; X32-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; X32-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0],xmm1[1,2,3],xmm0[4,5,6,7]
; X32-NEXT:    vmovq %xmm0, {{[0-9]+}}(%esp)
; X32-NEXT:    fildll {{[0-9]+}}(%esp)
; X32-NEXT:    fstps {{[0-9]+}}(%esp)
; X32-NEXT:    flds {{[0-9]+}}(%esp)
; X32-NEXT:    movl %ebp, %esp
; X32-NEXT:    popl %ebp
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_extract_uitofp:
; X64:       # BB#0:
; X64-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; X64-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0],xmm1[1,2,3],xmm0[4,5,6,7]
; X64-NEXT:    vmovq %xmm0, %rax
; X64-NEXT:    vcvtsi2ssq %rax, %xmm2, %xmm0
; X64-NEXT:    retq
  %1 = and <2 x i64> %a0, <i64 65535, i64 -1>
  %2 = extractelement <2 x i64> %1, i32 0
  %3 = uitofp i64 %2 to float
  ret float %3
}

define <4 x i32> @knownbits_mask_shuffle_sext(<8 x i16> %a0) nounwind {
; X32-LABEL: knownbits_mask_shuffle_sext:
; X32:       # BB#0:
; X32-NEXT:    vpand {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; X32-NEXT:    vpunpckhwd {{.*#+}} xmm0 = xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_shuffle_sext:
; X64:       # BB#0:
; X64-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; X64-NEXT:    vpunpckhwd {{.*#+}} xmm0 = xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; X64-NEXT:    retq
  %1 = and <8 x i16> %a0, <i16 -1, i16 -1, i16 -1, i16 -1, i16 15, i16 15, i16 15, i16 15>
  %2 = shufflevector <8 x i16> %1, <8 x i16> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  %3 = sext <4 x i16> %2 to <4 x i32>
  ret <4 x i32> %3
}

define <4 x float> @knownbits_mask_shuffle_uitofp(<4 x i32> %a0) nounwind {
; X32-LABEL: knownbits_mask_shuffle_uitofp:
; X32:       # BB#0:
; X32-NEXT:    vpand {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; X32-NEXT:    vcvtdq2ps %xmm0, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_shuffle_uitofp:
; X64:       # BB#0:
; X64-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; X64-NEXT:    vcvtdq2ps %xmm0, %xmm0
; X64-NEXT:    retq
  %1 = and <4 x i32> %a0, <i32 -1, i32 -1, i32 255, i32 4085>
  %2 = shufflevector <4 x i32> %1, <4 x i32> undef, <4 x i32> <i32 2, i32 2, i32 3, i32 3>
  %3 = uitofp <4 x i32> %2 to <4 x float>
  ret <4 x float> %3
}

define <4 x float> @knownbits_mask_or_shuffle_uitofp(<4 x i32> %a0) nounwind {
; X32-LABEL: knownbits_mask_or_shuffle_uitofp:
; X32:       # BB#0:
; X32-NEXT:    vpand {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpor {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; X32-NEXT:    vcvtdq2ps %xmm0, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_or_shuffle_uitofp:
; X64:       # BB#0:
; X64-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpor {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; X64-NEXT:    vcvtdq2ps %xmm0, %xmm0
; X64-NEXT:    retq
  %1 = and <4 x i32> %a0, <i32 -1, i32 -1, i32 255, i32 4085>
  %2 = or <4 x i32> %1, <i32 65535, i32 65535, i32 65535, i32 65535>
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> <i32 2, i32 2, i32 3, i32 3>
  %4 = uitofp <4 x i32> %3 to <4 x float>
  ret <4 x float> %4
}

define <4 x float> @knownbits_mask_xor_shuffle_uitofp(<4 x i32> %a0) nounwind {
; X32-LABEL: knownbits_mask_xor_shuffle_uitofp:
; X32:       # BB#0:
; X32-NEXT:    vpand {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpxor {{\.LCPI.*}}, %xmm0, %xmm0
; X32-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; X32-NEXT:    vcvtdq2ps %xmm0, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_xor_shuffle_uitofp:
; X64:       # BB#0:
; X64-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpxor {{.*}}(%rip), %xmm0, %xmm0
; X64-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; X64-NEXT:    vcvtdq2ps %xmm0, %xmm0
; X64-NEXT:    retq
  %1 = and <4 x i32> %a0, <i32 -1, i32 -1, i32 255, i32 4085>
  %2 = xor <4 x i32> %1, <i32 65535, i32 65535, i32 65535, i32 65535>
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> <i32 2, i32 2, i32 3, i32 3>
  %4 = uitofp <4 x i32> %3 to <4 x float>
  ret <4 x float> %4
}

define <4 x i32> @knownbits_mask_shl_shuffle_lshr(<4 x i32> %a0) nounwind {
; X32-LABEL: knownbits_mask_shl_shuffle_lshr:
; X32:       # BB#0:
; X32-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_shl_shuffle_lshr:
; X64:       # BB#0:
; X64-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; X64-NEXT:    retq
  %1 = and <4 x i32> %a0, <i32 -65536, i32 -7, i32 -7, i32 -65536>
  %2 = shl <4 x i32> %1, <i32 17, i32 17, i32 17, i32 17>
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> <i32 0, i32 0, i32 3, i32 3>
  %4 = lshr <4 x i32> %3, <i32 15, i32 15, i32 15, i32 15>
  ret <4 x i32> %4
}

define <4 x i32> @knownbits_mask_ashr_shuffle_lshr(<4 x i32> %a0) nounwind {
; X32-LABEL: knownbits_mask_ashr_shuffle_lshr:
; X32:       # BB#0:
; X32-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_ashr_shuffle_lshr:
; X64:       # BB#0:
; X64-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; X64-NEXT:    retq
  %1 = and <4 x i32> %a0, <i32 131071, i32 -1, i32 -1, i32 131071>
  %2 = ashr <4 x i32> %1, <i32 15, i32 15, i32 15, i32 15>
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> <i32 0, i32 0, i32 3, i32 3>
  %4 = lshr <4 x i32> %3, <i32 30, i32 30, i32 30, i32 30>
  ret <4 x i32> %4
}

define <4 x i32> @knownbits_mask_mul_shuffle_shl(<4 x i32> %a0, <4 x i32> %a1) nounwind {
; X32-LABEL: knownbits_mask_mul_shuffle_shl:
; X32:       # BB#0:
; X32-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_mul_shuffle_shl:
; X64:       # BB#0:
; X64-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; X64-NEXT:    retq
  %1 = and <4 x i32> %a0, <i32 -65536, i32 -7, i32 -7, i32 -65536>
  %2 = mul <4 x i32> %a1, %1
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> <i32 0, i32 0, i32 3, i32 3>
  %4 = shl <4 x i32> %3, <i32 22, i32 22, i32 22, i32 22>
  ret <4 x i32> %4
}

define <4 x i32> @knownbits_mask_trunc_shuffle_shl(<4 x i64> %a0) nounwind {
; X32-LABEL: knownbits_mask_trunc_shuffle_shl:
; X32:       # BB#0:
; X32-NEXT:    vandps {{\.LCPI.*}}, %ymm0, %ymm0
; X32-NEXT:    vextractf128 $1, %ymm0, %xmm1
; X32-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm1[4,5,6,7]
; X32-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[0,0,2,2]
; X32-NEXT:    vpslld $22, %xmm0, %xmm0
; X32-NEXT:    vzeroupper
; X32-NEXT:    retl
;
; X64-LABEL: knownbits_mask_trunc_shuffle_shl:
; X64:       # BB#0:
; X64-NEXT:    vandps {{.*}}(%rip), %ymm0, %ymm0
; X64-NEXT:    vextractf128 $1, %ymm0, %xmm1
; X64-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm1[4,5,6,7]
; X64-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[0,0,2,2]
; X64-NEXT:    vpslld $22, %xmm0, %xmm0
; X64-NEXT:    vzeroupper
; X64-NEXT:    retq
  %1 = and <4 x i64> %a0, <i64 -65536, i64 -7, i64 7, i64 -65536>
  %2 = trunc <4 x i64> %1 to <4 x i32>
  %3 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> <i32 0, i32 0, i32 3, i32 3>
  %4 = shl <4 x i32> %3, <i32 22, i32 22, i32 22, i32 22>
  ret <4 x i32> %4
}
