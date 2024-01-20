function main(): int32
{
    iteration: string = "Iteration\n";

    counter: int8 = 10;
    while counter > 0 {
        syscall3(1, 1, iteration.data, iteration.size);
        counter = counter - 1;
    }
    return 0;
}