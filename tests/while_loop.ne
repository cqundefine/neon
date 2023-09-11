function main(): int32
{
    counter: int8 = 10;
    while counter > 0 {
        syscall3(1, 1, to<int64>("Iteration\n"), 10);
        counter = counter - 1;
    }
    return 0;
}