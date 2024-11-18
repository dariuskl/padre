// This is free and unencumbered software released into the public domain.

#ifndef NONSTD_TEST_H
#define NONSTD_TEST_H

#include "nonstd.h"

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

#define NONSTD_COMPARISON(type)                                               \
  bool nonstd_compare_##type(type lhs, type rhs) {                            \
    return (lhs) == (rhs);                                                    \
  }                                                                           \
  bool nonstd_compare_ ## type ## ptr(const type *lhs, const type *rhs) {     \
    return (lhs) == (rhs);                                                    \
  }

NONSTD_COMPARISON(i8)
NONSTD_COMPARISON(u8)
NONSTD_COMPARISON(i16)
NONSTD_COMPARISON(u16)
NONSTD_COMPARISON(i32)
NONSTD_COMPARISON(u32)

#define TEST_COMPARE_EQ(lhs, rhs) (_Generic((lhs),                            \
    i32: nonstd_compare_i32,                                                  \
    const u8 *: nonstd_compare_u8ptr,                                         \
    utf8: utf8_eq                                                             \
  )(lhs, rhs))

#define TEST_PRINT(v) (_Generic((v),                                          \
    i32: print_i32,                                                           \
    u8 *: print_ptr,                                                          \
    const u8 *: print_ptr,                                                    \
    utf8: print                                                               \
  )(v))

#define TEST_ASSERT_EQ(actual, expected) do {                                 \
  if (!TEST_COMPARE_EQ(actual, expected)) {                                   \
    print(utf8("  compared values not equal\n    actual: "));                 \
    TEST_PRINT(actual);                                                       \
    print(utf8("\n   vs.\n    expected: "));                                  \
    TEST_PRINT(expected);                                                     \
    print(utf8("\n"));                                                        \
    TEST_FAIL();                                                              \
  }                                                                           \
} while (0)

#define TEST_ASSERT_NE(actual, expected) do {                                 \
  if (TEST_COMPARE_EQ(actual, expected)) {                                    \
    print(utf8("  compared values equal\n    actual: "));                     \
    TEST_PRINT(actual);                                                       \
    print(utf8("\n   vs.\n    expected: "));                                  \
    TEST_PRINT(expected);                                                     \
    print(utf8("\n"));                                                        \
    TEST_FAIL();                                                              \
  }                                                                           \
} while (0)

#define TEST_ASSERT_EQ_RANGE(actual_begin, actual_end,                        \
                             expected_begin, expected_end)                    \
for (byte *itr_a = actual_begin, *itr_e = expected_begin;                     \
     itr_a != actual_end; ++itr_a, ++itr_e) {                                 \
  if (itr_a == actual_end) {                                                  \
    TEST_FAIL();                                                              \
  }                                                                           \
  if (*itr_a != *expected_begin) {                                            \
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
  test_state.file = filename;
}

void nonstd_run_test(void func(void), utf8 funcname, int line) {
  test_state.func = funcname;
  test_state.line = line;
  ++test_state.n_tests;
  test_state.failed = false;
  func();
  print(funcname);
  if (test_state.failed) {
    ++test_state.n_failures;
    println(" .................. FAILED");
  } else {
    println(" .................. OK");
  }
}

#define BEGIN_TESTS(argc, argv, envp)                                         \
  (nonstd_begin_tests(argc, argv, envp, to_utf8(__FILE__)))

#define RUN_TEST(func)                                                        \
  (nonstd_run_test(func, utf8(# func), __LINE__))

#define END_TESTS()                    test_state.n_failures

#endif // NONSTD_TEST_H
