include "Syscalls"

const STDIN_FILENO: int32 = 0;
const STDOUT_FILENO: int32 = 1;
const STDERR_FILENO: int32 = 2;

const SEEK_SET: int32 = 0;
const SEEK_CUR: int32 = 1;
const SEEK_END: int32 = 2;

function read(fd: int32, buf: uint8*, count: uint64): int64 // FIXME: Use ssize_t instead of int64
{
    return syscall3(SYS_read, fd, to<uint64>(buf), count);
}

function write(fd: int32, buf: uint8*, count: uint64): int64 // FIXME: Use ssize_t instead of int64
{
    return syscall3(SYS_write, fd, to<uint64>(buf), count);
}

function open(file: string, flags: int32, mode: uint32): int32 // FIXME: Use mode_t instead of uint32
{
    return syscall3(SYS_open, to<uint64>(file.data), flags, mode);
}

function close(fd: int32): int32
{
    return syscall1(SYS_close, fd);
}

function lseek(fd: int32, offset: int64, whence: int32): int64
{
    return syscall3(SYS_lseek, fd, offset, whence);
}
