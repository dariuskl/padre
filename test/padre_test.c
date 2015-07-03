/*
 *   Copyright 2015 Darius Kellermann
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
 
/** Unit tests for functions from the file `padre.c`.
 * @file	padre_test.c
 * @author	Darius Kellermann <darius.kellermann@gmail.com>
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "../src/padre.c"

#define GRAPH	"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define KLEENE	"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define ALNUM	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define ALPHA	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGIT	"0123456789"
#define LOWER	"abcdefghijklmnopqrstuvwxyz"
#define PUNCT	"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define UPPER	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define WORD	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"
#define XDIGIT	"ABCDEFabcdef0123456789"

static int test_to_pwdchars(uint8_t *str, size_t len, char *chars, char *expected)
{
	to_pwdchars(str, len, chars, strlen(chars));
	if (strcmp((char *)str, expected) != 0) {
		fprintf(stderr, "\t\t: Expected `%s`, got `%s`\n", expected, str);
	}
	else {
		puts("\t\t: Success");
		return 0;
	}

	return -1;
}

static int tests_for_to_pwdchars()
{
	int result;
	int ret;
	uint8_t str[95];
	char *chars;
	size_t clen;

	result = 0;

	for (size_t i = 0; i <= sizeof str; i++) {
		str[i] = i;
	}

	ret = enumerate_charset("*", &chars, &clen);
	if (ret != 0)
		return -1;

	result |= test_to_pwdchars(str, (sizeof str) - 1, chars, chars);

	free(chars);

	if (result == 0)
		errno = 0;

	return result;
}

static int test_enumerate_charset(char *class, char *expected)
{
	int ret;
	char *res;
	size_t rlen;

	printf("\ttesting character class `%s` ...\n", class);
	errno = 0;
	ret = enumerate_charset(class, &res, &rlen);
	if (ret != 0) {
		perror("\t\t");
	}
	else if (res == NULL || rlen == 0) {
		fputs("\t\t: Illegal return value(s)\n", stderr);
	}
	else if (strcmp(res, expected) != 0) {
		fprintf(stderr, "\t\t: Expected `%s`, got `%s`\n", expected, res);
	}
	else if (rlen != strlen(expected)) {
		fprintf(stderr, "\t\t: Expected `%zu`, got `%zu`\n", strlen(expected), rlen);
	}
	else {
		puts("\t\t: Success");
		return 0;
	}

	free(res);

	return -1;
}

static int tests_for_enumerate_charset()
{
	int result;
	int ret;

	result = 0;

	ret = enumerate_charset(NULL, (char **)1, (size_t *)1);
	if (!(ret < 0 && errno == EINVAL)) {
		perror("\t\t");
		result |= -1;
	}

	ret = enumerate_charset(NULL, NULL, NULL);
	if (!(ret < 0 && errno == EINVAL)) {
		perror("\t\t");
		result |= -1;
	}

	/* Test character classes
	 */
	result |= test_enumerate_charset(":graph:", GRAPH);
	result |= test_enumerate_charset(":alnum:", ALNUM);
	result |= test_enumerate_charset(":alpha:", ALPHA);
	result |= test_enumerate_charset(":digit:", DIGIT);
	result |= test_enumerate_charset(":lower:", LOWER);
	result |= test_enumerate_charset(":punct:", PUNCT);
	result |= test_enumerate_charset(":upper:", UPPER);
	result |= test_enumerate_charset(":word:", WORD);
	result |= test_enumerate_charset(":xdigit:", XDIGIT);
	result |= test_enumerate_charset("*", KLEENE);

	result |= test_enumerate_charset(":alnum", ":alnum");

	if (result == 0)
		errno = 0;

	return result;
}

int padre_test_execute()
{
	int result;
	int ret;

	result = 0;

	puts("Testing enumerate_charset() ...");
	ret = tests_for_enumerate_charset();
	if (ret != 0 && errno == 0)
		fputs("\t: Failure\n", stderr);
	else
		perror("\t");
	result |= ret;

	puts("Testing to_pwdchars() ...");
	ret = tests_for_to_pwdchars();
	if (ret != 0 && errno == 0)
		fputs("\t: Failure\n", stderr);
	else
		perror("\t");
	result |= ret;

	return result;
}
