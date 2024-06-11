include "File"
include "Malloc"
include "Standard"

function main(argc: int32, argv: int8**): int32
{
    if argc != 2 {
        print("Usage: cat <file>\n");
        return 1;
    }

    print(*from_cstr(argv[1]));

    var fd: int32 = open(*from_cstr(argv[1]), 0, 0);
    if fd < 0 {
        print("Failed to open file\n");
        return 1;
    }

    var size: int64 = lseek(fd, 0, SEEK_END) + 1;
    lseek(fd, 0, SEEK_SET);

    var buffer: uint8* = malloc(1024);
    
    var bytesRead: int32 = read(fd, buffer, size);
    if bytesRead < 0 {
        print("Failed to read file\n");
        return 1;
    }

    write(STDOUT_FILENO, buffer, bytesRead);

    free(buffer);
    close(fd);
    return 0;
}
