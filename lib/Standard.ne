function print(text: string): void
{
    syscall3(1, 1, text.data, text.size);
}