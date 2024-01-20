function main(): int32
{
    text: string = "Hello, world!\n";
    syscall3(1, 1, text.data, text.size);
    return 0;
}