// This is free and unencumbered software released into the public domain.

// see https://github.com/skeeto/u-config/blob/master/linux_amd64_main.c

enum {
  SYSCALL_read = 0,
  SYSCALL_write = 1,
  SYSCALL_open = 2,
  SYSCALL_close = 3,
  SYSCALL_mmap = 9,
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

#include "linux.c"
