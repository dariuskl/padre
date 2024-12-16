// This is free and unencumbered software released into the public domain.

// see https://github.com/skeeto/u-config/blob/master/linux_amd64_main.c

enum {
  SYSCALL_read = 0,
  SYSCALL_write = 1,
  SYSCALL_open = 2,
  SYSCALL_close = 3,
  SYSCALL_fstat = 5,
  SYSCALL_mmap = 9,
  SYSCALL_munmap = 11,
  SYSCALL_exit = 60,
};

__asm__ (
  "         .globl _start       \n"
  "_start:  mov   %rsp, %rdi    \n"
  "         call  start         \n"
);

static long syscall1(long n, long a) {
  long r;
  __asm__ volatile ("syscall" : "=a"(r)
                              : "a"(n), "D"(a)
                              : "rcx", "r11", "memory");
  return r;
}

static long syscall2(long n, long a, long b) {
  long r;
  __asm__ volatile ("syscall" : "=a"(r)
                              : "a"(n), "D"(a), "S"(b)
                              : "rcx", "r11", "memory");
  return r;
}

static long syscall3(long n, long a, long b, long c) {
  long r;
  __asm__ volatile ("syscall" : "=a"(r)
                              : "a"(n), "D"(a), "S"(b), "d"(c)
                              : "rcx", "r11", "memory");
  return r;
}

static long syscall6(long n, long a, long b, long c, long d, long e, long f) {
  long r;
  register long r10 __asm__ ("r10") = d;
  register long r8  __asm__ ("r8")  = e;
  register long r9  __asm__ ("r9")  = f;
  __asm__ volatile ("syscall" : "=a"(r)
                              : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
                              : "rcx", "r11", "memory");
  return r;
}

typedef struct {
  u64 st_dev;
  u64 st_ino;
  u64 st_nlink;
  unsigned st_mode;
  unsigned st_uid;
  unsigned st_gid;
  int xxpad0;
  u64 st_rdev;
  i64 st_size;
  i64 st_blksize;
  i64 st_blocks;
  u64 st_atime_;
  u64 st_atime_nsec_;
  u64 st_mtime_;
  u64 st_mtime_nsec_;
  u64 st_ctime_;
  u64 st_ctime_nsec_;
  i64 xxreserved[3];
} stat_amd64;

size get_file_size(int fd) {
  stat_amd64 stat;
  long ret = syscall2(SYSCALL_fstat, fd, (long)&stat);
  if (ret < 0)
    return -1;
  return min(stat.st_size, N_SIZE_MAX);
}

#include "linux.c"
