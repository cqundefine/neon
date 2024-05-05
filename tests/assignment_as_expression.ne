include "Standard"

function main(): int32
{
    var num: uint16 = 2;
    if num = 0 {
        print("This should not get printed\n");
    }

    return 0;
}
