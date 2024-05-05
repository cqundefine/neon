include "Standard"

function main(): int32
{
    var counter: int8 = 10;
    while counter > 0 {
        print("Iteration\n");
        counter = counter - 1;
    }
    return 0;
}
