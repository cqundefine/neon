function main(): int32
{
    *"test" = 10;
    // You can derefrence a number but you have to cast it to a pointer
    *0x0 = 10;
}