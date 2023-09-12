target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@"Hello, world!\0A" = constant [14 x i8] c"Hello, world!\0A"

define i64 @syscall0(i64 %0) {
entry:
  %1 = call i64 asm sideeffect alignstack inteldialect "syscall", "={ax},{ax},~{memory},~{dirflag},~{fpsr},~{flags}"(i64 %0)
  ret i64 %1
}

define i64 @syscall1(i64 %0, i64 %1) {
entry:
  %2 = call i64 asm sideeffect alignstack inteldialect "syscall", "={ax},{ax},{di},~{memory},~{dirflag},~{fpsr},~{flags}"(i64 %0, i64 %1)
  ret i64 %2
}

define i64 @syscall2(i64 %0, i64 %1, i64 %2) {
entry:
  %3 = call i64 asm sideeffect alignstack inteldialect "syscall", "={ax},{ax},{di},{si},~{memory},~{dirflag},~{fpsr},~{flags}"(i64 %0, i64 %1, i64 %2)
  ret i64 %3
}

define i64 @syscall3(i64 %0, i64 %1, i64 %2, i64 %3) {
entry:
  %4 = call i64 asm sideeffect alignstack inteldialect "syscall", "={ax},{ax},{di},{si},{dx},~{memory},~{dirflag},~{fpsr},~{flags}"(i64 %0, i64 %1, i64 %2, i64 %3)
  ret i64 %4
}

define i32 @main() {
entry:
  %call = call i64 @syscall3(i1 false, i1 false, i64 ptrtoint ([14 x i8]* @"Hello, world!\0A" to i64), i14 64)
  ret i0 0
}
