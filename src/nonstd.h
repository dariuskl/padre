// This is free and unencumbered software released into the public domain.

// Much inspiration from https://nullprogram.com/blog/2023/10/08.

#ifndef NONSTD_H
#define NONSTD_H

typedef unsigned char     byte;  // can be used to inspect object representation
typedef   signed char       i8;
typedef unsigned char       u8;
typedef  __INT16_TYPE__    i16;
typedef __UINT16_TYPE__    u16;
typedef  __INT32_TYPE__    i32;
typedef __UINT32_TYPE__    u32;
typedef  __INT64_TYPE__    i64;
typedef __UINT64_TYPE__    u64;
typedef __UINTPTR_TYPE__  uptr;  // use to store addresses
typedef __PTRDIFF_TYPE__  size;  // preferred size type
typedef __SIZE_TYPE__    usize;  // for compatibility with size_t

// There is no benefit in having values live in types smaller than a register,
// so the boolean type is not `_Bool` but rather just `unsigned`.
#define bool    unsigned
#define false   0
#define true    1

// While I currently prefer to just use 0, nullptr certainly is the next-best.
#if __STDC_VERSION__ < 202311L
  #define nullptr ((void *)0)
#endif

#if __STDC_VERSION__ < 201112L
  #define noreturn
#elif __STDC_VERSION < 202311L
  #define noreturn _Noreturn
#else
  #define noreturn [[noreturn]]
#endif

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define size_of(obj)   ((size)sizeof(obj))
#define count_of(arr)  (size_of(arr) / size_of((arr)[0]))

#define assert(cond) while (!(cond)) __builtin_trap()

// memory operations

static inline void *copy_b(const void *src_begin, const void *src_end,
                           void *dst_begin, const void *dst_end) {
  const byte *src_it = src_begin;
  byte *dst_it = dst_begin;
  for (; src_it != src_end && dst_it != dst_end; ++src_it, ++dst_it) {
    *dst_it = *src_it;
  }
  return dst_it;
}

#define memcpy(dest, src, count)                                              \
  copy_b((src), (const byte *)(src) + (count),                                \
         (dest), (const byte *)(dest) + (count))

static inline void fill(byte *begin, const byte *end, byte v) {
  for (; begin != end; ++begin)
    *begin = v;
}

#define memset(dest, ch, count)                                               \
  fill((byte *)(dest), ((byte *)(dest) + (count)), (byte)(ch))

static inline void clear_b(byte *begin, const byte *end) {
  fill(begin, end, 0);
}

#define clear(begin, end)                                                     \
  clear_b((byte *)(begin), (const byte *)(end))

#define clear_n(begin, count)                                                 \
  clear((byte *)(begin), ((const byte *)(begin) + (count)))

static inline void clear_s(byte *begin, const byte *end) {
  for (volatile byte *volatile itr = begin; itr != end; ++itr) {
    *itr = 0;
  }
}

#define clear_s_n(begin, count)                                               \
  clear_s((byte *)(begin), (const byte *)(begin) + (count))

static inline bool equal_b(const byte *lhs_begin, const byte *lhs_end,
                           const byte *rhs_begin, const byte *rhs_end) {
  if (lhs_end - lhs_begin != rhs_end - rhs_begin)
    return false;

  for (; lhs_begin != lhs_end; ++lhs_begin, ++rhs_begin)
    if (*lhs_begin != *rhs_begin)
      return false;

  return true;
}

#define equal_b_n(lhs, rhs, count)                                            \
  equal_b((const byte *)(lhs), ((const byte *)(lhs) + (count)),               \
          (const byte *)(rhs), ((const byte *)(rhs) + (count)))

// ASCII strings

static inline size ascii_length_of(const char *s) {
    const char *end = s;
    for (; *end; ++end) {
    }
    return end - s;
}

static inline int ascii_cmp(const char *lhs, const char *rhs) {
  for (; *lhs == *rhs && *lhs; lhs++, rhs++) {
  }
  return *lhs - *rhs;
}

// UTF-8 strings

#define utf8(s)    ((utf8){(const u8 *)(u8 ## s),                             \
                           (const u8 *)(&u8 ## s[0] + sizeof(u8 ## s) - 1)})
#define to_utf8(s) ((utf8){(const u8 *)(s),                                   \
                           (const u8 *)(&s[0] + ascii_length_of((const char *)(s)))})

typedef struct {
  const u8 *begin;
  const u8 *end;
} utf8;

static inline bool utf8_empty(utf8 s) {
  return s.begin == s.end;
}

// The size of the given string in bytes.
// Not necessarily the same as the number of characters in the string.
static inline size utf8_len(utf8 s) {
  return s.end - s.begin;
}

static inline bool utf8_eq(utf8 lhs, utf8 rhs) {
  if (lhs.end - lhs.begin != rhs.end - rhs.begin)
    return false;

  for (; *lhs.begin != *lhs.end; ++lhs.begin, ++rhs.begin)
    if (*lhs.begin != *rhs.begin)
      return false;

  return true;
}

static inline bool utf8_is_ascii_digit(char c) {
  return c >= '0' && c <= '9';
}

static inline bool utf8_is_ascii_space(u32 c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline utf8 utf8_trim(utf8 s) {
  for (; s.begin != s.end && utf8_is_ascii_space(*s.begin); ++s.begin) {
  }
  for (; s.begin != s.end && utf8_is_ascii_space(*(s.end - 1)); --s.end) {
  }
  return s;
}

static inline bool utf8_startswith(utf8 str, utf8 pat) {
  if (str.end - str.begin < pat.end - pat.begin)
    return false;

  for (; *pat.begin != *pat.end; ++str.begin, ++pat.begin)
    if (*str.begin != *pat.begin)
      return false;

  return true;
}

static inline u32 utf8_nextch(utf8 *str) {
  size len = str->end - str->begin;
  if (!len)
    return 0;

  u32 c = *str->begin;
  if (!(c & 0x80)) {
    str->begin += 1;
    return c;  // ascii character
  }
  if (!(c & 0x40)) {
    str->begin += 1;
    return U'�';  // continuation byte where none is expected
  }
  if (!(c & 0x20)) {
    if (len < 2) {
      str->begin += len;
      return U'�';  // truncated
    }
    c = (str->begin[0] & 0x1f) << 6 | (str->begin[1] & 0x3f);
    str->begin += 2;
    return c;
  }
  if (!(c & 0x10)) {
    if (len < 3) {
      str->begin += len;
      return U'�'; // truncated
    }
    c = (str->begin[0] & 0x0f) << 12 | (str->begin[1] & 0x3f) << 6
        | (str->begin[2] & 0x3f);
    str->begin += 3;
    return c;
  }
  if (!(c & 0x08)) {
    if (len < 4) {
      str->begin += len;
      return U'�'; // truncated
    }
    c = (str->begin[0] & 0x07) << 18 | (str->begin[1] & 0x3f) << 12
           | (str->begin[2] & 0x3f) << 6 | (str->begin[3] & 0x3f);
    str->begin += 4;
    return c;
  }
  str->begin += 1;
  return U'�';
}

typedef struct {
  u8 *begin;
  u8 *eod;        // one past the end of data
  const u8 *end;  // one past the end of buffer
} buf8;

#define buf8(a)   ((buf8){.begin = (a), .eod = (a), .end = (a) + sizeof(a)})

static inline size buf8_used(buf8 b) {
  return b.eod - b.begin;
}

static inline size buf8_capacity(buf8 b) {
  return b.end - b.begin;
}

static inline bool buf8_push(buf8 *b, u32 c) {
  size cap = buf8_capacity(*b);
  if (c < 0x80 && cap >= 1) {
    b->eod[0] = c & 0x7f;
    b->eod += 1;
    return true;
  }
  if (c < 0x800 && cap >= 2) {
    b->eod[0] = 0xc0 | ((c >> 6) & 0x1f);
    b->eod[1] = 0x80 | (c & 0x3f);
    b->eod += 2;
    return true;
  }
  if (c < 0x10000 && cap >= 3) {
    b->eod[0] = 0xe0 | ((c >> 12) & 0x0f);
    b->eod[1] = 0x80 | ((c >> 6) & 0x3f);
    b->eod[2] = 0x80 | (c & 0x3f);
    b->eod += 3;
    return true;
  }
  if (c < 0x10ffff && cap >= 4) {
    b->eod[0] = 0xf0 | ((c >> 18) & 0x07);
    b->eod[1] = 0x80 | ((c >> 12) & 0x3f);
    b->eod[2] = 0x80 | ((c >> 6) & 0x3f);
    b->eod[3] = 0x80 | (c & 0x3f);
    b->eod += 4;
    return true;
  }
  if (cap >= 3) {
    b->eod[0] = 0xef;
    b->eod[1] = 0xbf;
    b->eod[2] = 0xbd;
    b->eod += 3;
    return true;
  }
  return false;
}

static inline void buf8_print(buf8 *buf, utf8 s) {
  for (; buf->eod != buf->end && s.begin != s.end; ++buf->eod, ++s.begin) {
    *buf->eod = *s.begin;
  }
}

static inline void buf8_print_i(buf8 *buf, int i) {
  if (buf->eod == buf->end)
    return;

  if (i == 0) {
    *buf->eod++ = '0';
    return;
  }

  if (i < 0) {
    *buf->eod++ = '-';
    i = -i;
  }

  for (; i > 0; i /= 10) {
    *buf->eod++ = (u8)('0' + (i % 10));
  }
}

typedef struct {
  const u8 *begin;
  const u8 *end;  // one past the end of data
} view8;

static inline size view8_len(view8 v) {
  return v.end - v.begin;
}

static inline view8 buf_to_view(buf8 b) {
  return (view8){b.begin, b.eod};
}

static inline view8 utf8_to_view(utf8 s) {
  return (view8){s.begin, s.end};
}

// File I/O (OPTIONAL)

int open_file(utf8 filename, int mode);
void close_file(int fd);
size get_file_size(int fd);
size read_from_file(int fd, byte *begin, const byte *end);
size write_to_file(int fd, const byte *begin, const byte *end);

// Text I/O (OPTIONAL)

size print_to_file(utf8 text, int fd);
size print_i32(i32 num);
size print_u8(u8 num);
size print_ptr(const void *ptr);

// print to stdout
static inline size print(utf8 text) {
  return print_to_file(text, 1);
}
#define println(s)    print_to_file(utf8(s "\n"), 1)

// returns the number of bytes read from fd,
//   0 in case of EOF,
//  -1 for errors
size scan_from_file(buf8 *buf, int fd);
bool scan_i32(utf8 *buf, i32 *num);

#define scan(s)       scan_from_file(s, 0)  // scan from stdin

// Program entry and exit (OPTIONAL)
//
// Instead of writing a standard C main(), you can use below signature and one
// of the platform-specific *.c files (e.g., linux_amd64.c) as the program's
// entry point.

// Same as the signature for main() EXCEPT argv[0] is the first argument
// instead of the program call.
i32 entry(i32 argc, u8 *argv[], u8 *envp[]);

noreturn void exit_with_failure(void);

// Dynamic memory management (OPTIONAL)
//
// Instead of using the standard C dynamic memory management,

// Allocates memory of at least `n_bytes` size.
// Exits on failure.
__attribute__((malloc))
byte *os_allocate(size n_bytes);

typedef struct {
  byte *begin;
  byte *end;
} arena;

static inline arena new_arena(size n_bytes) {
  byte *ptr = os_allocate(n_bytes);
  return (arena){ptr, ptr + n_bytes};
}

__attribute__((malloc))
static inline void *allocate(arena *a, int num_objs, size obj_size) {
  const size available = a->end - a->begin;
  if (available < 0 || num_objs > available / obj_size)
    exit_with_failure();
  void *p = a->begin;
  a->begin += num_objs * obj_size;
  return p;
}

#define new(a, n, t) ((t *)allocate(a, n, sizeof(t)))

#endif // NONSTD_H
