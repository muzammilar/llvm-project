; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 2
; RUN: llc -mtriple=amdgcn -mcpu=gfx1010 -amdgpu-atomic-optimizer-strategy=None < %s | FileCheck %s -check-prefix=GCN

define amdgpu_cs void @should_not_hoist_set_inactive(<4 x i32> inreg %i14, i32 inreg %v, i32 %lane, i32 %f, i32 %f2) #0 {
; GCN-LABEL: should_not_hoist_set_inactive:
; GCN:       ; %bb.0: ; %.entry
; GCN-NEXT:    v_cmp_eq_u32_e64 s5, 0, v0
; GCN-NEXT:    v_cmp_ne_u32_e64 s6, 0, v2
; GCN-NEXT:    s_mov_b32 s7, 0
; GCN-NEXT:    v_cmp_gt_i32_e32 vcc_lo, 3, v1
; GCN-NEXT:    s_branch .LBB0_2
; GCN-NEXT:  .LBB0_1: ; %bb4
; GCN-NEXT:    ; in Loop: Header=BB0_2 Depth=1
; GCN-NEXT:    s_waitcnt_depctr 0xffe3
; GCN-NEXT:    s_or_b32 exec_lo, exec_lo, s8
; GCN-NEXT:    s_and_b32 s8, exec_lo, s6
; GCN-NEXT:    s_or_b32 s7, s8, s7
; GCN-NEXT:    s_andn2_b32 exec_lo, exec_lo, s7
; GCN-NEXT:    s_cbranch_execz .LBB0_5
; GCN-NEXT:  .LBB0_2: ; %bb
; GCN-NEXT:    ; =>This Inner Loop Header: Depth=1
; GCN-NEXT:    s_and_saveexec_b32 s8, vcc_lo
; GCN-NEXT:    s_cbranch_execz .LBB0_1
; GCN-NEXT:  ; %bb.3: ; %bb1
; GCN-NEXT:    ; in Loop: Header=BB0_2 Depth=1
; GCN-NEXT:    s_or_saveexec_b32 s9, -1
; GCN-NEXT:    v_cndmask_b32_e64 v3, 0, s4, s9
; GCN-NEXT:    v_mov_b32_e32 v4, 0
; GCN-NEXT:    v_mov_b32_dpp v4, v3 row_xmask:1 row_mask:0xf bank_mask:0xf
; GCN-NEXT:    s_mov_b32 exec_lo, s9
; GCN-NEXT:    v_mov_b32_e32 v0, v4
; GCN-NEXT:    s_and_b32 exec_lo, exec_lo, s5
; GCN-NEXT:    s_cbranch_execz .LBB0_1
; GCN-NEXT:  ; %bb.4: ; %bb2
; GCN-NEXT:    ; in Loop: Header=BB0_2 Depth=1
; GCN-NEXT:    buffer_atomic_add v0, off, s[0:3], 0
; GCN-NEXT:    s_branch .LBB0_1
; GCN-NEXT:  .LBB0_5: ; %bb5
; GCN-NEXT:    s_endpgm
.entry:
  br label %bb

bb:
  %i17 = icmp slt i32 %f, 3
  br i1 %i17, label %bb1, label %bb4

bb1:
  %i25 = call i32 @llvm.amdgcn.set.inactive.i32(i32 %v, i32 0)
  %i26 = call i32 @llvm.amdgcn.update.dpp.i32(i32 0, i32 %i25, i32 353, i32 15, i32 15, i1 false)
  %i38 = call i32 @llvm.amdgcn.strict.wwm.i32(i32 %i26)
  %i39 = icmp eq i32 %lane, 0
  br i1 %i39, label %bb2, label %bb3

bb2:
  %i41 = call i32 @llvm.amdgcn.raw.buffer.atomic.add.i32(i32 %i38, <4 x i32> %i14, i32 0, i32 0, i32 0)
  br label %bb3

bb3:
  br label %bb4

bb4:
  %exit = icmp eq i32 %f2, 0
  br i1 %exit, label %bb, label %bb5

bb5:
  ret void
}

declare i32 @llvm.amdgcn.set.inactive.i32(i32, i32)
declare i32 @llvm.amdgcn.update.dpp.i32(i32, i32, i32 immarg, i32 immarg, i32 immarg, i1 immarg)
declare i32 @llvm.amdgcn.strict.wwm.i32(i32)
declare i32 @llvm.amdgcn.raw.buffer.atomic.add.i32(i32, <4 x i32>, i32, i32, i32 immarg)
