include "Standard"

function main(): int32
{
    var: uint16 = 2;
    if var = 0 {
        print("This should not get printed\n");
    }

    return 0;
}
