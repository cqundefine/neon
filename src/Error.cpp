#include <Error.h>
#include <Lexer.h>
#include <stdarg.h>
#include <stdio.h>

extern std::string g_FileName;
extern Lexer g_Lexer;

[[noreturn]] void Error(uint32_t offset, const char* fmt, ...)
{
    auto location = g_Lexer.LineColumnFromOffset(offset);
    fprintf(stderr, "%s:%d:%d ", g_FileName.c_str(),location.first, location.second);

    va_list va;
    va_start(va, fmt);

    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);

    exit(1);
}
