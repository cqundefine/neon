include "File"
include "Standard"

function main(argc: int32, argv: int8**): int32
{
    var counter: int32 = 0;
    while counter < argc {
        const size: int64 = strlen(argv[counter]);
        write(STDOUT_FILENO, argv[counter], size);
        print("\n");
        counter = counter + 1;
    }
    return 0;
}
