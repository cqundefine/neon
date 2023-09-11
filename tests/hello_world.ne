function main(): int32
{
    syscall3(1, 1, to<int64>("Hello, world!\n"), 14);
    return 0;
}