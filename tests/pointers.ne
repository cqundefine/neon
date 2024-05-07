include "Malloc"

function main(): int32
{
    var ptr: int32* = to<int32*>(malloc(4));
    *ptr = 12;
    var returnValue: int32 = *ptr;
    free(ptr);
    return returnValue;
}
