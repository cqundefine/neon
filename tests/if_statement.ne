function main(): int32
{
    var: int8 = 2;
    if var == 2 {
        syscall3(1, 1, "It works!\n", 10);
    } else {
        syscall3(1, 1, "Error!\n", 7);
    }
    return 0;
}