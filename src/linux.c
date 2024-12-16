// This is free and unencumbered software released into the public domain.

#include "nonstd.h"

// Program entry and exit

__attribute__((used))
void start(long *stack) {
  i32 argc = (i32)stack[0] - 1;  // minus argv[0]
  u8 **argv = (u8 **)stack + 2;  // plus one to skip argv[0]
  u8 **envp = argv + argc + 1;
  i32 status = entry(argc, argv, envp);
  syscall1(SYSCALL_exit, status);
  __builtin_unreachable();
}

void exit_with_failure(void) {
  syscall1(SYSCALL_exit, 1);
  __builtin_unreachable();
}

byte *os_allocate(size n_bytes) {
  byte *ptr = (void *)syscall6(SYSCALL_mmap, 0, n_bytes,
                               3, // rw
                               0x22, // priv, anon
                               -1, 0);
  if (ptr == (void *)-1) {
    exit_with_failure();
  }
  return ptr;
}

int open_file(utf8 filename, int mode) {
  // FIXME filename could be not zero-terminated
  return (int)syscall2(SYSCALL_open, (long)filename.begin, mode);
}

void close_file(int fd) {
  if (fd != 0 && fd != 1 && fd != 2) // != stdin, stdout, stderr
    syscall1(SYSCALL_close, fd);
}

size read_from_file(int fd, byte *begin, const byte *end) {
  size len = end - begin;
  if (len <= 0)
    return -1;
  return syscall3(SYSCALL_read, fd, (long)begin, len);
}

size write_to_file(int fd, const byte *begin, const byte *end) {
  size len = end - begin;
  if (len <= 0)
    return -1;
  return syscall3(SYSCALL_write, fd, (long)begin, len);
}

buf8 map_file(utf8 filename, int mode) {
  int fd = open_file(filename, mode);
  if (fd <= 0)
    return (buf8){};
  size file_size = get_file_size(fd);
  int prot = 0;
  switch (mode) {
  case 0: // read-only
    prot = 1;
    break;
  default:
    return (buf8){};
  }
  byte *ptr = (void *)syscall6(SYSCALL_mmap, 0, file_size, prot, 2, // private
                               fd, 0);
  // map the file into memory
  if (ptr == (byte *)-1) {
    return (buf8){};
  }
  return (buf8){ptr, ptr + file_size, ptr + file_size};
}

int unmap_file(buf8 b) {
  return (int)syscall2(SYSCALL_munmap, (long)b.begin, buf8_capacity(b));
}

size print_to_file(utf8 text, int fd) {
  return syscall3(SYSCALL_write, fd, (long)text.begin, utf8_len(text));
}

size scan_from_file(buf8 *buf, int fd) {
  long ret = syscall3(SYSCALL_read, fd, (long)buf->eod, buf->end - buf->eod);
  buf->eod += ret > 0 ? ret : 0;
  return ret;
}
