; ModuleID = 'int.bc'

; Function Attrs: nounwind ssp uwtable
define i32 @int4pl(i32 %a, i32 %b) {
  %1 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %1, 0
  %ofl = extractvalue { i32, i1 } %1, 1
  br i1 %ofl, label %arith_overflow_block, label %arith_non_overflow_block

  arith_non_overflow_block:                         ; preds = %entry
   ret i32 %val

 arith_overflow_block:                             ; preds = %entry
   ret i32 0
}

; Function Attrs: nounwind ssp uwtable
define i32 @int4mi(i32 %a, i32 %b) {
  %1 = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %1, 0
  %ofl = extractvalue { i32, i1 } %1, 1
  br i1 %ofl, label %arith_overflow_block, label %arith_non_overflow_block

  arith_non_overflow_block:                         ; preds = %entry
   ret i32 %val

 arith_overflow_block:                             ; preds = %entry
   ret i32 0
}

; Function Attrs: nounwind ssp uwtable
define i32 @int4mul(i32 %a, i32 %b) {
  %1 = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %a, i32 %b)
  %val = extractvalue { i32, i1 } %1, 0
  %ofl = extractvalue { i32, i1 } %1, 1
  br i1 %ofl, label %arith_overflow_block, label %arith_non_overflow_block

  arith_non_overflow_block:                         ; preds = %entry
   ret i32 %val

 arith_overflow_block:                             ; preds = %entry
   ret i32 0
}

declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32)
declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32)
declare { i32, i1 } @llvm.smul.with.overflow.i32(i32, i32)

