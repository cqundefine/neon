extern function hello(): void;

function main(): int32
{
    var: int32 = 2 + 1;
    if var == 2 {
        return 1;
    } else {
        return 0;
    }
    hello();
    return 2;
}