include "Standard"

struct A
{
    a: uint8;
    b: uint8;
}

function modifyA(a: A): void
{
    a.a = a.a + 1;
}

function main(): int32
{
    a: A;
    a.a = 1;
    a.b = 2;
    modifyA(a);
    return a.a + a.b;
} 