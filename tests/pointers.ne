// FIXME: On 32-bit the size is int32
extern function malloc(size: int64): int32*;
extern function free(ptr: int32*): void;

function main(): int32
{
    var ptr: int32* = malloc(4);
    *ptr = 12;
    var returnValue: int32 = *ptr;
    free(ptr);
    return returnValue;
}
