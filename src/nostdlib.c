// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <https://unlicense.org/>

// Much inspiration from https://nullprogram.com/blog/2023/10/08.
// Components contained in this amalgamation: utf8 file test compat

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

#define N_I16_MIN   ((i16)-0x8000)
#define N_I16_MAX   0x7fff
#define N_U16_MAX   0xffff
#define N_I32_MIN   ((i32)-0x80000000)
#define N_I32_MAX   0x7fffffff
#define N_U32_MAX   0xffffffff
#define N_I64_MIN   (-0x8000000000000000)
#define N_I64_MAX   0x7fffffffffffffff
#define N_U64_MAX   0xffffffffffffffff
#define N_SIZE_MAX  __PTRDIFF_MAX__

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

#define clamp(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))

#define size_of(obj)   ((size)sizeof(obj))
#define count_of(arr)  (size_of(arr) / size_of((arr)[0]))

// The desired behavior for an assertion would be to
// - break, when a debugger is connected;
// - exit, otherwise.
// Support for that is spotty across architectures, and always requires `asm`.
void break_or_exit(void);
#define assert(cond) while (!(cond)) break_or_exit()

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

static inline void fill(byte *begin, const byte *end, byte v) {
  for (; begin != end; ++begin)
    *begin = v;
}

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

typedef struct {
  u8 *begin;
  u8 *eod;        // one past the end of data
  const u8 *end;  // one past the end of buffer
} buf8;

#define buf8(a)   ((buf8){.begin = (a), .eod = (a), .end = (a) + sizeof(a)})

static inline size buf8_used(buf8 b) {
  return b.eod - b.begin;
}

static inline size buf8_available(buf8 b) {
  return b.end - b.eod;
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

// terminates the string with zero without allocating that as a used byte
static inline void buf8_0term(buf8 *buf) {
  if (buf->eod != buf->end) {
    *buf->eod = '\0';
    ++buf->eod;
  }
}

void buf8_print_i32(buf8 *buf, i32 v);
void buf8_print_i64(buf8 *buf, i64 v);
void buf8_print_u8(buf8 *buf, u8 v);
void buf8_print_u32(buf8 *buf, u32 v);
void buf8_print_u64(buf8 *buf, u64 v);
void buf8_print_i(buf8 *buf, int v);

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
  buf8 buf;
} arena;

static inline arena new_arena(size n_bytes) {
  byte *ptr = os_allocate(n_bytes);
  return (arena){{ptr, ptr, ptr + n_bytes}};
}

// Stack-like allocator. Pushes by the given number of bytes and returns a
// buffer of the allocated data.
// Returns empty buffer on failure.
static inline buf8 arena_try_push(arena *a, size n_bytes) {
  const size available = buf8_available(a->buf);
  if (available < n_bytes)
    return (buf8){};
  u8 *p = a->buf.eod;
  a->buf.eod += n_bytes;
  return (buf8){p, p, a->buf.eod};
}

// See `arena_try_push`.
// Exits on failure.
static inline buf8 arena_push(arena *a, size n_bytes) {
  buf8 ret = arena_try_push(a, n_bytes);
  if (ret.begin == 0)
    exit_with_failure();
  return ret;
}

static inline void arena_reset(arena *a) {
  a->buf.eod = a->buf.begin;
}

buf8 arena_push_aligned();

#define new(a, n, t) ((t *)arena_push(a, n * size_of(t)).eod)
// UTF-8 strings

// Converts a string literal to a `utf8`-typed string.
#define utf8(s)    ((utf8){(const u8 *)(u8 ## s),                             \
                           (const u8 *)(&u8 ## s[0] + sizeof(u8 ## s) - 1)})
// Converts a C-string to a `utf8`-typed string.
#define to_utf8(s) ((utf8){(const u8 *)(s),                                   \
                           (const u8 *)(&s[0] + ascii_length_of((const char *)(s)))})

// A proper string type that does not rely on zero-termination.
// Assumes(!) proper UTF-8 encoding.
// Note that this is a view, to manipulate strings refer to `buf8`.
typedef struct {
  const u8 *begin;  // pointer to the beginning of the string
  const u8 *end;    // pointer one past the last byte of the string
} utf8;

static inline utf8 buf_to_utf8(buf8 b) {
  return (utf8){b.begin, b.eod};
}

static inline view8 utf8_to_view(utf8 s) {
  return (view8){s.begin, s.end};
}

static inline void buf8_print(buf8 *buf, utf8 s) {
  for (; buf->eod != buf->end && s.begin != s.end; ++buf->eod, ++s.begin) {
    *buf->eod = *s.begin;
  }
}

bool scan_i32(utf8 *str, i32 *num);
bool scan_i64(utf8 *str, i64 *num);

// returns the remainder of `str` after scanning the number
// the operation was successful only if the returned string is not equal to the
// original string
utf8 scan_u8(utf8 str, u8 *num);
bool scan_u64(utf8 str, u64 *num);

static inline bool utf8_empty(utf8 s) {
  return s.begin >= s.end;
}

// The size of the given string in bytes.
// Not necessarily the same as the number of characters in the string.
// Can yield negative values for invalid strings.
static inline size utf8_size(utf8 s) {
  return s.end - s.begin;
}

static inline bool utf8_eq(utf8 lhs, utf8 rhs) {
  if (utf8_size(lhs) != utf8_size(rhs))
    return false;

  for (; lhs.begin < lhs.end; ++lhs.begin, ++rhs.begin)
    if (*lhs.begin != *rhs.begin)
      return false;

  return true;
}

static inline bool utf8_is_ascii_digit(u32 c) {
  return c >= '0' && c <= '9';
}

static inline bool utf8_is_ascii_space(u32 c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// returns a substring of `s` that omits ASCII spaces at the beginning and end
static inline utf8 utf8_trimmed(utf8 s) {
  // iterating byte-wise is valid here because we're only interested in
  // ASCII spaces
  for (; s.begin < s.end && utf8_is_ascii_space(*s.begin); ++s.begin) {
  }
  for (; s.begin < s.end && utf8_is_ascii_space(*(s.end - 1)); --s.end) {
  }
  return s;
}

static inline bool utf8_startswith(utf8 str, utf8 pat) {
  if (utf8_size(str) < utf8_size(pat))
    return false;

  for (; pat.begin < pat.end; ++str.begin, ++pat.begin)
    if (*str.begin != *pat.begin)
      return false;

  return true;
}

u32 utf8_nextch(utf8 *str);

typedef struct {
  utf8 head;
  utf8 tail;
} utf8_parts;

// splits the string along the first occurrence of the separator
// if there is no separator, tail will be empty
utf8_parts utf8_split(utf8 str, u32 separator);
utf8_parts utf8_splits(utf8 str, utf8 separator);

static inline buf8 buf8_from_hex(arena *a, utf8 str) {
  buf8 ret = arena_push(a, utf8_size(str) / 2);
  while (!utf8_empty(str)) {
    utf8 tail = scan_u8(str, ret.eod);
    if (utf8_eq(tail, str))
      break;
    ++ret.eod;
    str = tail;
  }
  return ret;
}

// File I/O (OPTIONAL)

// mode: 0 - read-only,
//       1 - write-only,
//       2 - read/write,
int open_file(utf8 filename, int mode);
void close_file(int fd);
size get_file_size(int fd);
size read_from_file(int fd, byte *begin, const byte *end);
size write_to_file(int fd, const byte *begin, const byte *end);

buf8 map_file(utf8 filename, int mode);
int unmap_file(buf8 b);

// Text I/O (OPTIONAL)

size print_to_file(utf8 text, int fd);

// returns the number of bytes read from fd,
//   0 in case of EOF,
//  -1 for errors
size scan_from_file(buf8 *buf, int fd);

// scan from stdin

#define scan(s)       scan_from_file(s, 0)  // scan from stdin

// print to stdout

size print_i32(i32 num);
size print_i64(i64 num);
size print_u8(u8 num);
size print_u32(u32 num);
size print_u64(u64 num);
size print_ptr(const void *ptr);

static inline size print_str(utf8 s) { return print_to_file(s, 1); }

#define print(s)      print_str(s)
#define println(s)    print_str(utf8(s "\n"))

// log to stderr

size n_log_i8(i8 num);
size n_log_i16(i16 num);
size n_log_i32(i32 num);
size n_log_i64(i64 num);

size n_log_u8(u8 num);
size n_log_u16(u16 num);
size n_log_u32(u32 num);
size n_log_u64(u64 num);

#define n_log(s)      print_to_file(s, 2)
#define n_logln(s)    n_log(utf8(s "\n"))

#define TEST_FAIL() do {                                                      \
  nonstd_fail_test(to_utf8(__FILE__), __LINE__);                              \
  return;                                                                     \
} while (0)

#define TEST_ASSERT_TRUE(v) do {                                              \
  if (!(v)) {                                                                 \
    TEST_FAIL();                                                              \
  }                                                                           \
} while (0)

#define TEST_ASSERT_FALSE(v) do {                                             \
  if (v) {                                                                    \
    TEST_FAIL();                                                              \
  }                                                                           \
} while (0)

#define N_COMPARISON(type)                                                    \
  bool nonstd_compare_##type(type lhs, type rhs) {                            \
    return (lhs) == (rhs);                                                    \
  }                                                                           \
  bool nonstd_compare_ ## type ## ptr(const type *lhs, const type *rhs) {     \
    return (lhs) == (rhs);                                                    \
  }

static inline bool nonstd_compare_voidptr(const void *lhs, const void *rhs) {
  return lhs == rhs;
}

N_COMPARISON(i8)
N_COMPARISON(u8)
N_COMPARISON(i16)
N_COMPARISON(u16)
N_COMPARISON(i32)
N_COMPARISON(u32)
N_COMPARISON(i64)
N_COMPARISON(u64)

#define TEST_COMPARE_EQ(lhs, rhs) (_Generic((lhs),                            \
    i32: nonstd_compare_i32,                                                  \
    u32: nonstd_compare_u32,                                                  \
    i64: nonstd_compare_i64,                                                  \
    u64: nonstd_compare_u64,                                                  \
    const void *: nonstd_compare_voidptr,                                     \
    const u8 *: nonstd_compare_u8ptr,                                         \
    utf8: utf8_eq                                                             \
  )(lhs, rhs))

#define TEST_PRINT(v) (_Generic((v),                                          \
    u8: print_u8,                                                             \
    i32: print_i32,                                                           \
    u32: print_u32,                                                           \
    i64: print_i64,                                                           \
    u64: print_u64,                                                           \
    const void *: print_ptr,                                                  \
    u8 *: print_ptr,                                                          \
    const u8 *: print_ptr,                                                    \
    utf8: print_str                                                           \
  )(v))

#define TEST_ASSERT_EQ(actual, expected) do {                                 \
  if (!TEST_COMPARE_EQ((actual), (expected))) {                               \
    print(utf8("  compared values not equal\n      actual: "));               \
    TEST_PRINT(actual);                                                       \
    print(utf8("\n    vs.\n      expected: "));                               \
    TEST_PRINT(expected);                                                     \
    print(utf8("\n"));                                                        \
    TEST_FAIL();                                                              \
  }                                                                           \
} while (0)

#define TEST_ASSERT_NE(actual, expected) do {                                 \
  if (TEST_COMPARE_EQ(actual, expected)) {                                    \
    print(utf8("  compared values equal\n      actual: "));                   \
    TEST_PRINT(actual);                                                       \
    print(utf8("\n   vs.\n    expected: "));                                  \
    TEST_PRINT(expected);                                                     \
    print(utf8("\n"));                                                        \
    TEST_FAIL();                                                              \
  }                                                                           \
} while (0)

// FIXME print size not i32
#define TEST_ASSERT_EQ_RANGE(actual_begin, actual_end,                        \
                             expected_begin, expected_end)                    \
for (size i = 0, alen = (actual_end) - (actual_begin),                        \
                 elen = (expected_end) - (expected_begin);                    \
     i < alen || i < elen; ++i) {                                             \
  if (i == alen || i == elen) {                                               \
    println("  compared ranges have different length");                       \
    TEST_FAIL();                                                              \
  }                                                                           \
  if ((actual_begin)[i] != (expected_begin)[i]) {                             \
    print(utf8("  compared ranges not equal at pos: "));                      \
    print_i32((i32)i);                                                        \
    print(utf8("\n      actual: "));                                          \
    TEST_PRINT((actual_begin)[i]);                                            \
    print(utf8("\n   vs.\n    expected: "));                                  \
    TEST_PRINT((expected_begin)[i]);                                          \
    print(utf8("\n"));                                                        \
    TEST_FAIL();                                                              \
  }                                                                           \
}

struct {
  utf8 file;
  utf8 func;
  int line;
  int n_tests;
  int n_failures;
  bool failed;
  bool is_tty;
} test_state;

void nonstd_fail_test(utf8 file, int line) {
  test_state.failed = true;
  print(utf8("  in "));
  print(file);
  print(utf8(", line "));
  print_i32(line);
  print(utf8("\n"));
  print(utf8("  called from "));
  print(test_state.func);
  print(utf8(" in "));
  print(test_state.file);
  print(utf8(", line "));
  print_i32(test_state.line);
  print(utf8("\n"));
}

void nonstd_begin_tests(i32 argc, u8 *argv[], u8 *envp[], utf8 filename) {
  (void)argc, (void)argv, (void)envp;
  int tty_fd = open_file(utf8("/dev/tty"), 2);
  test_state.is_tty = tty_fd >= 0;
  test_state.file = filename;
  print(filename);
  println();
}

void nonstd_run_test(void func(void), utf8 funcname, int line) {
  test_state.func = funcname;
  test_state.line = line;
  ++test_state.n_tests;
  test_state.failed = false;
  func();
  print(utf8("      "));
  print(funcname);
  print(utf8("\r"));
  if (test_state.failed) {
    ++test_state.n_failures;
    print(utf8("\r " "\033[31;1m" "FAIL" "\033[0m" " "));
  } else {
    print(utf8("\r " "\033[32;1m" " OK " "\033[0m" " "));
  }
  print(funcname);
  println();
}

#define BEGIN_TESTS(argc, argv, envp)                                         \
  (nonstd_begin_tests(argc, argv, envp, to_utf8(__FILE__)))

#define RUN_TEST(func)                                                        \
  (nonstd_run_test(func, utf8(# func), __LINE__))

#define END_TESTS()                    test_state.n_failures

#define memcpy(dest, src, count)                                              \
  copy_b((src), (const byte *)(src) + (count),                                \
         (dest), (const byte *)(dest) + (count))

#define memset(dest, ch, count)                                               \
  fill((byte *)(dest), ((byte *)(dest) + (count)), (byte)(ch))

#ifdef NOSTDLIB_IMPLEMENTATION
u32 utf8_nextch(utf8 *str) {
  size sz = utf8_size(*str);
  if (sz <= 0)
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
    if (sz < 2) {
      str->begin += sz;
      return U'�';  // truncated
    }
    c = (str->begin[0] & 0x1f) << 6 | (str->begin[1] & 0x3f);
    str->begin += 2;
    return c;
  }
  if (!(c & 0x10)) {
    if (sz < 3) {
      str->begin += sz;
      return U'�'; // truncated
    }
    c = (str->begin[0] & 0x0f) << 12 | (str->begin[1] & 0x3f) << 6
        | (str->begin[2] & 0x3f);
    str->begin += 3;
    return c;
  }
  if (!(c & 0x08)) {
    if (sz < 4) {
      str->begin += sz;
      return U'�'; // truncated
    }
    c = (u32)(str->begin[0] & 0x07) << 18
      | (u32)(str->begin[1] & 0x3f) << 12
      | (u32)(str->begin[2] & 0x3f) << 6
      | (u32)(str->begin[3] & 0x3f);
    str->begin += 4;
    return c;
  }
  str->begin += 1;
  return U'�';
}

utf8_parts utf8_split(utf8 str, u32 separator) {
  utf8 head = str;
  for (u32 c = 0; (c = utf8_nextch(&str)); head.end = str.begin) {
    if (c == separator)
      return (utf8_parts){head, str};
  }
  return (utf8_parts){head, str};
}

utf8_parts utf8_splits(utf8 str, utf8 separator) {
  utf8 head = str;
  for (; str.begin < str.end; ++str.begin) {
    if (utf8_startswith(str, separator))
      return (utf8_parts){(utf8){head.begin, str.begin},
                          (utf8){str.begin + utf8_size(separator), str.end}};
  }
  return (utf8_parts){head, str};
}
static u8 chars[] = "0123456789abcdef";

static size print_uint(u64 num, int field_width, u8 filler, int fd) {
  u8 buf_[16];
  buf8 buf = buf8(buf_);
  buf.eod = buf.begin + 16;

  if (num == 0) {
    --buf.eod;
    *buf.eod = '0';
  } else {
    for (; num > 0; num /= 16) {
      --buf.eod;
      *buf.eod = chars[num % 16];
    }
  }

  while (buf.eod > buf.end - field_width) {
    --buf.eod;
    *buf.eod = filler;
  }

  return print_to_file(((utf8){buf.eod, buf.end}), fd);
}

static void buf8_print_uint(buf8 *b, u64 num, int base, int field_width, u8 filler) {
  if (base < 2 || base > 16 || field_width < 0)
    return;

  if (buf8_capacity(*b) < (field_width ? field_width : 16))
    return;

  u8 *wp = b->eod;
  // initially, print the number right-aligned
  wp += field_width ? field_width : 16;
  u8 *end = wp;

  if (num == 0) {
    --wp;
    *wp = '0';
  } else {
    for (; num > 0; num /= (unsigned)base) {
      --wp;
      *wp = chars[num % (unsigned)base];
    }
  }

  // left-align number and pad with filler
  if (field_width) {
    while (b->eod < wp) {
      *b->eod = filler;
      ++b->eod;
    }
    b->eod = end;
  } else for (u8 *eod = b->eod + (end - wp); b->eod < eod; ++b->eod, ++wp)
    *b->eod = *wp;
}

void buf8_print_u8(buf8 *buf, u8 v) {
  buf8_print_uint(buf, v, 16, 2, '0');
}

void buf8_print_u32(buf8 *buf, u32 v) {
  buf8_print_uint(buf, v, 16, 8, '0');
}

void buf8_print_u64(buf8 *buf, u64 v) {
  buf8_print_uint(buf, v, 16, 16, '0');
}

static size print_int(i64 num, int field_width, u8 filler, int fd) {
  u8 buf_[16];
  buf8 buf = buf8(buf_);
  buf.eod = buf.begin + 16;

  if (num == 0) {
    --buf.eod;
    *buf.eod = '0';
  } else {
    u64 absv = (u64)(num < 0 ? -num : num);

    for (; absv > 0; absv /= 10) {
      --buf.eod;
      *buf.eod = chars[absv % 10];
    }

    if (num < 0) {
      --buf.eod;
      *buf.eod = '-';
    }
  }

  while (buf.eod > buf.end - field_width) {
    --buf.eod;
    *buf.eod = filler;
  }

  return print_to_file(((utf8){buf.eod, buf.end}), fd);
}

static void buf8_print_int(buf8 *b, i64 num, int base, int field_width, u8 filler) {
  if (base < 2 || base > 16 || field_width < 0)
    return;

  if (buf8_capacity(*b) < (field_width ? field_width : 16))
    return;

  u8 *wp = b->eod;
  // initially, print the number right-aligned
  wp += field_width ? field_width : 16;
  u8 *end = wp;

  if (num == 0) {
    --wp;
    *wp = '0';
  } else {
    u64 absv = (u64)(num < 0 ? -num : num);

    for (; absv > 0; absv /= (unsigned)base) {
      --wp;
      *wp = chars[absv % (unsigned)base];
    }

    if (num < 0) {
      --wp;
      *wp = '-';
    }
  }

  // left-align number and pad with filler
  if (field_width) {
    while (b->eod < wp) {
      *b->eod = filler;
      ++b->eod;
    }
    b->eod = end;
  } else for (u8 *eod = b->eod + (end - wp); b->eod < eod; ++b->eod, ++wp)
    *b->eod = *wp;
}

void buf8_print_i32(buf8 *buf, i32 v) {
  buf8_print_int(buf, v, 10, 0, '0');
}

void buf8_print_i64(buf8 *buf, i64 v) {
  buf8_print_int(buf, v, 10, 0, '0');
}

void buf8_print_i(buf8 *buf, int v) {
  buf8_print_int(buf, v, 10, 0, '0');
}

size print_i32(i32 num) {
  return print_int(num, 0, u8'0', 1);
}

size print_i64(i64 num) {
  return print_int(num, 0, u8'0', 1);
}

size print_u8(u8 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 1);
}

size print_u32(u32 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 1);
}

size print_u64(u64 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 1);
}

size print_ptr(const void *ptr) {
  return print_uint((uptr)ptr, 16, 0, u8'0');
}

size n_log_i8(i8 num) {
  return print_int(num, 0, u8'0', 2);
}

size n_log_i16(i16 num) {
  return print_int(num, 0, u8'0', 2);
}

size n_log_i32(i32 num) {
  return print_int(num, 0, u8'0', 2);
}

size n_log_i64(i64 num) {
  return print_int(num, 0, u8'0', 2);
}

size n_log_u8(u8 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 2);
}

size n_log_u16(u16 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 2);
}

size n_log_u32(u32 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 2);
}

size n_log_u64(u64 num) {
  return print_uint(num, sizeof(num) * 2, u8'0', 2);
}

bool scan_i32(utf8 *str, i32 *num) {
  i64 temp;
  if (!scan_i64(str, &temp))
    return false;
  if (temp > N_I32_MAX || temp < N_I32_MIN)
    return false;
  *num = (i32)temp;
  return true;
}

bool scan_i64(utf8 *str, i64 *num) {
  bool ret = false;
  i64 tmp = 0;
  for (; str->begin < str->end && utf8_is_ascii_digit(*str->begin);
       ++str->begin) {
    tmp = tmp * 10 + (i64)*str->begin - '0';
    ret = true;
  }
  if (ret)
    *num = tmp;
  return ret;
}

utf8 scan_u8(utf8 str, u8 *num) {
  int tmp = 0;
  const u8 *end = min(str.begin + 2, str.end);
  for (; str.begin < end; ++str.begin) {
    if (utf8_is_ascii_digit(*str.begin)) {
      tmp = tmp * 16 + *str.begin - '0';
    } else if (*str.begin >= 'A' && *str.begin <= 'F') {
      tmp = tmp * 16 + (*str.begin - 'A') + 10;
    } else if (*str.begin >= 'a' && *str.begin <= 'f') {
      tmp = tmp * 16 + (*str.begin - 'a') + 10;
    }
  }
  *num = (u8)tmp;
  return str;
}

bool scan_u64(utf8 str, u64 *num) {
  u64 tmp = 0;
  for (; str.begin < str.end; ++str.begin) {
    if (utf8_is_ascii_digit(*str.begin)) {
      tmp = tmp * 16 + (u64)*str.begin - '0';
    } else if (*str.begin >= 'A' && *str.begin <= 'F') {
      tmp = tmp * 16 + (*str.begin - 'A') + 10;
    } else if (*str.begin >= 'a' && *str.begin <= 'f') {
      tmp = tmp * 16 + (*str.begin - 'a') + 10;
    }
  }
  *num = tmp;
  return true;
}
#endif

// Determines the system-specific implementations and startup-files to be used.
// Include this file from the file containing your executable's entry point.

#if __linux__
  #if __amd64__
void break_or_exit(void) {
  // the nop helps GDB because int3 breaks at the next instruction
  __asm__ volatile ("int3; nop");
}

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
  #endif
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
  break_or_exit();
  syscall1(SYSCALL_exit, 1); // TODO this is already unreachable
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
  return syscall3(SYSCALL_write, fd, (long)text.begin, utf8_size(text));
}

size scan_from_file(buf8 *buf, int fd) {
  long ret = syscall3(SYSCALL_read, fd, (long)buf->eod, buf->end - buf->eod);
  buf->eod += ret > 0 ? ret : 0;
  return ret;
}
#elif __MSP430__
// TODO
#else
  #error "unsupported"
#endif
