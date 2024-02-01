include "Standard"

function main(argc: int32, argv: int8**): int32
{
    counter: int32 = 0;
    while counter < argc {
        print(String(args[counter]));
        counter = counter + 1;
    }
}