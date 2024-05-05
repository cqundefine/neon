include "Syscalls"

function print(text: string): void
{
    syscall3(SYS_write, 1, to<uint64>(text.data), text.size);
}
