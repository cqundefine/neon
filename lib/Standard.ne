include "File"
include "Malloc"

extern function strlen(str: int8*): int64;  // FIXME: On 32-bit the size is int32

function from_cstr(cstr: int8*): string*
{
    var s: string* = to<string*>(malloc(STRING_SIZE));
    s.data = cstr;
    s.size = strlen(cstr);
    return s;
}

function print(text: string): void
{
    write(STDOUT_FILENO, text.data, text.size);
}
