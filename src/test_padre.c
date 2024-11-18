// This is free and unencumbered software released into the public domain.

#include "padre.c"

#include "nonstd_test.h"

#define GRAPH                                                                  \
  "!\"#$%&'()*+,-./"                                                           \
  "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"                         \
  "abcdefghijklmnopqrstuvwxyz{|}~"
#define KLEENE                                                                 \
  "!\"#$%&'()*+,-./"                                                           \
  "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"                         \
  "abcdefghijklmnopqrstuvwxyz{|}~"
#define ALNUM "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGIT "0123456789"
#define LOWER "abcdefghijklmnopqrstuvwxyz"
#define PUNCT "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define WORD "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"
#define XDIGIT "ABCDEFabcdef0123456789"

void test_to_chars(buf8 bytes, utf8 chars, utf8 expected) {
  utf8 actual = to_chars(bytes, chars);
  TEST_ASSERT_NE(actual.begin, bytes.begin);
  TEST_ASSERT_EQ(actual, expected);
}

void tests_for_to_chars(void) {
  utf8 chars = enumerate_charset(utf8("*"));
  u8 str[95];

  for (size i = 0; i < size_of(str); ++i) {
    str[i] = (u8)i;
  }

  test_to_chars(buf8(str), chars, chars);
}

void test_enumerate_charset(utf8 spec, utf8 expected) {
  print(utf8("\ttesting character spec `"));
  print(spec);
  println("` ...");

  utf8 actual = enumerate_charset(spec);
  TEST_ASSERT_EQ(actual, expected);
}

void tests_for_enumerate_charset(void) {
  // null spec, *
  utf8 actual = enumerate_charset((utf8){});
  TEST_ASSERT_EQ(actual, to_utf8(KLEENE));

  // empty spec, *
  actual = enumerate_charset(utf8(""));
  TEST_ASSERT_EQ(actual, to_utf8(KLEENE));

  // test character classes
  test_enumerate_charset(utf8(":graph:"), to_utf8(GRAPH));
  test_enumerate_charset(utf8(":alnum:"), to_utf8(ALNUM));
  test_enumerate_charset(utf8(":alpha:"), to_utf8(ALPHA));
  test_enumerate_charset(utf8(":digit:"), to_utf8(DIGIT));
  test_enumerate_charset(utf8(":lower:"), to_utf8(LOWER));
  test_enumerate_charset(utf8(":punct:"), to_utf8(PUNCT));
  test_enumerate_charset(utf8(":upper:"), to_utf8(UPPER));
  test_enumerate_charset(utf8(":word:"), to_utf8(WORD));
  test_enumerate_charset(utf8(":xdigit:"), to_utf8(XDIGIT));
  test_enumerate_charset(utf8("*"), to_utf8(KLEENE));

  // test broken inputs
  test_enumerate_charset(utf8(":alnum"), utf8(":alnum"));

  // test ranges
  test_enumerate_charset(utf8("a-z"), utf8("abcdefghijklmnopqrstuvwxyz"));

  // test for repetition - TODO find a way to fold redundant chars
#if 0
  test_enumerate_charset("a-za-za-za-za-za-za-za-za-za-za-za-za-za-za-za-za-z",
                         "abcdefghijklmnopqrstuvwxyz");
#endif
}

i32 entry(i32 argc, u8 *argv[], u8 *envp[]) {
  BEGIN_TESTS(argc, argv, envp);
  RUN_TEST(tests_for_to_chars);
  RUN_TEST(tests_for_enumerate_charset);
  return END_TESTS();
}

#include "nonstd.c"
