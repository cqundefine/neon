function print(text: string): void
{
    syscall3(1, 1, to<int64>(text.data), text.size);
}