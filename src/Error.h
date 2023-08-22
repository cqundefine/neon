#pragma once

#include <stdint.h>

[[noreturn]] void Error(uint32_t offset, const char* fmt, ...);
