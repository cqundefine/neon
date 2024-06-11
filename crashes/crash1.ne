include "Standard"

struct A
{
    a:  int8; b: uint8;
}

function modifyA(a: A): void
{ a.a = a.a + 1;
}

function main(): int32
{
var a: A;
    a.a = 1;
    a.b = 2;
modifyA(a = a);
return a.a + a.b;
} 