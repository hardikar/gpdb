; ModuleID = 'int.bc'
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.10.0"

; Function Attrs: nounwind ssp uwtable
define i32 @int8pl(i32 %a, i32 %b) {
  %1 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %1, 0
  %ofl = extractvalue { i32, i1 } %1, 1
  br i1 %ofl, label %arith_overflow_block, label %arith_non_overflow_block

 arith_non_overflow_block:                         ; preds = %entry
   ret i32 %val

 arith_overflow_block:                             ; preds = %entry
   ret i32 0
}

declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32)

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"Apple LLVM version 7.0.2 (clang-700.1.81)"}
