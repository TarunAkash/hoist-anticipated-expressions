; ModuleID = 'test/test2.cpp'
source_filename = "test/test2.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress noinline norecurse nounwind optnone uwtable
define dso_local noundef i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 0, ptr %retval, align 4
  store i32 5, ptr %x, align 4
  store i32 10, ptr %y, align 4
  store i32 0, ptr %result, align 4
  %0 = load i32, ptr %x, align 4
  %cmp = icmp sgt i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32, ptr %x, align 4
  %2 = load i32, ptr %y, align 4
  %mul = mul nsw i32 %1, %2
  %3 = load i32, ptr %result, align 4
  %add = add nsw i32 %3, %mul
  store i32 %add, ptr %result, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %4 = load i32, ptr %x, align 4
  %5 = load i32, ptr %y, align 4
  %mul1 = mul nsw i32 %4, %5
  %6 = load i32, ptr %result, align 4
  %sub = sub nsw i32 %6, %mul1
  store i32 %sub, ptr %result, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %7 = load i32, ptr %result, align 4
  ret i32 %7
}

attributes #0 = { mustprogress noinline norecurse nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 22.0.0git (https://github.com/llvm/llvm-project.git 9ad7edef4276207ca4cefa6b39d11145f4145a72)"}
