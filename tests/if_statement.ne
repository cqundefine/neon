function main(): int32
{
    itWorks: string = "It works!\n";
    error: string = "Error!\n";

    var: int8 = 2;
    if var == 2 {
        syscall3(1, 1, itWorks.data, itWorks.size);
    } else {
        syscall3(1, 1, error.data, error.size);
    }
    return 0;
}