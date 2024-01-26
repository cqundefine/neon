include "Standard"

function test(param: uint64): void
{
    if param == 123 {
        return;
    }

    print("Unreachable\n");
}

function main(): int32
{
    test(123);
    return 0;
}
