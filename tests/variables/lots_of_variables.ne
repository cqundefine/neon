function add(a: int32, b: int32): int32
{
    return a + b;
}

function main(): int32
{
    var a: int32 = 1;
    var b: int32 = 2;
    var c: int32 = 3;
    b = add(a, b);
    var d: int32 = 4;
    var e: int32 = 5;
    e = a + b + c + d;
    d = add(e, a);
    c = add(d, b);
    e = add(c, d);
    return e;
}
