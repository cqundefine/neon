include "Standard"

struct A
{
    a: uint8;
    b: uint8;
}

function main(): int32
{
    a: A;
    a.a = 1;
    a.b = 2;
    return a.a + a.b;
}