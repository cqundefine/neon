function print(text: string): void
{
#if X86_64
    const SYS_write: uint64 = 1;
#endif
#if AArch64
    const SYS_write: uint64 = 64;
#endif
    syscall3(SYS_write, 1, to<int64>(text.data), text.size);
}
