//
//   Copyright 2015 Darius Kellermann
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#include "padre.c"

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

#include <assert.h>
#include <errno.h>
#include <stdio.h>

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

static void test_to_pwdchars(char *str, const size_t len, char *chars,
                             const char *expected) {
  to_pwdchars(str, len, chars, strlen(chars));
  TEST_ASSERT_EQUAL_STRING(expected, str);
}

static void tests_for_to_pwdchars(void) {
  char *chars;
  size_t clen;
  const int ret = enumerate_charset("*", &chars, &clen);
  TEST_ASSERT_EQUAL(0, ret);

  char str[95];

  for (size_t i = 0; i < sizeof str; ++i) {
    str[i] = (char)i;
  }

  test_to_pwdchars(str, sizeof str - 1, chars, chars);

  free(chars);
}

static void test_enumerate_charset(char *class, const char *expected) {
  printf("\ttesting character class `%s` ...\n", class);

  char *res;
  size_t rlen;
  const int ret = enumerate_charset(class, &res, &rlen);
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT_NOT_EQUAL(0, rlen);
  TEST_ASSERT_EQUAL_STRING(expected, res);
  TEST_ASSERT_EQUAL(strlen(expected), rlen);

  free(res);
}

static void tests_for_enumerate_charset(void) {
  errno = 0;
  int ret = enumerate_charset(NULL, (char **)1, (size_t *)1);
  TEST_ASSERT_LESS_THAN(0, ret);
  TEST_ASSERT_EQUAL(EINVAL, errno);

  errno = 0;
  ret = enumerate_charset(NULL, NULL, NULL);
  TEST_ASSERT_LESS_THAN(0, ret);
  TEST_ASSERT_EQUAL(EINVAL, errno);

  // test character classes
  test_enumerate_charset(":graph:", GRAPH);
  test_enumerate_charset(":alnum:", ALNUM);
  test_enumerate_charset(":alpha:", ALPHA);
  test_enumerate_charset(":digit:", DIGIT);
  test_enumerate_charset(":lower:", LOWER);
  test_enumerate_charset(":punct:", PUNCT);
  test_enumerate_charset(":upper:", UPPER);
  test_enumerate_charset(":word:", WORD);
  test_enumerate_charset(":xdigit:", XDIGIT);
  test_enumerate_charset("*", KLEENE);

  test_enumerate_charset(":alnum", ":alnum");
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(tests_for_enumerate_charset);
  RUN_TEST(tests_for_to_pwdchars);
  return UNITY_END();
}