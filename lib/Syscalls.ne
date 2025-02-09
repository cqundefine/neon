extern function syscall0(syscall: uint64): uint64;
extern function syscall1(syscall: uint64, arg1: uint64): uint64;
extern function syscall2(syscall: uint64, arg1: uint64, arg2: uint64): uint64;
extern function syscall3(syscall: uint64, arg1: uint64, arg2: uint64, arg3: uint64): uint64;
extern function syscall4(syscall: uint64, arg1: uint64, arg2: uint64, arg3: uint64, arg4: uint64): uint64;
extern function syscall5(syscall: uint64, arg1: uint64, arg2: uint64, arg3: uint64, arg4: uint64, arg5: uint64): uint64;
extern function syscall6(syscall: uint64, arg1: uint64, arg2: uint64, arg3: uint64, arg4: uint64, arg5: uint64, arg6: uint64): uint64; 

#if X86_64
    const SYS_read: uint64 = 0;
    const SYS_write: uint64 = 1;
    const SYS_open: uint64 = 2;
    const SYS_close: uint64 = 3;
    const SYS_lseek: uint64 = 8;
#endif
#if AArch64
    const SYS_read: uint64 = 63;
    const SYS_write: uint64 = 64;
    const SYS_open: uint64 = 257;
    const SYS_close: uint64 = 57;
    const SYS_lseek: uint64 = 62;
#endif
