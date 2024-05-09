#if X86_64
    const SYS_read: uint64 = 0;
    const SYS_write: uint64 = 1;
    const SYS_open: uint64 = 2;
    const SYS_close: uint64 = 3;
    const SYS_lseek: uint64 = 8;
#endif
#if AArch64
    const SYS_write: uint64 = 64;
#endif
