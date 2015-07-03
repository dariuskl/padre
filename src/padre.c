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
 
/** Command-line input parsing and implementation.
 * @file	padre.c
 * @author	Darius Kellermann <darius.kellermann@gmail.com>
 */

#include <argp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <crypto/crypto_scrypt.h>

#include "padre.h"

static char *to_pwdchars(uint8_t *str, size_t len, char *chars, size_t clen);
static int enumerate_charset(const char *def, char **res, size_t *rlen);

/**
 * Asks the user for his master password, stores it in `passwd` and updates the
 * length in `len`.
 *
 * \param[in,out]	len	The length of the buffer resp. the length of the
 *				read password string not including the
 *				terminating null byte.
 *
 * \return	0 on success, -1 in case of a failure.
 */
static int ask_password(char *passwd, size_t *len);

static struct argp_option argp_options[]  = {
		{
			"length", 'l', "64", 0,
			"Length of the generated password."
		},
		{
			"passno", 'n', "0", 0,
			"Password iteration number."
		},
		{
			"chars", 'c', ":graph:", 0,
			"List of characters or the name of a POSIX character "
			"class to use in the generated password (regexp "
			"notation)."
		},
		{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	int tmp;

	switch (key) {
	case 'c':
		options.chars = arg;
		break;
	case 'l':
		tmp = atoi(arg);
		if (tmp == 0 || tmp < 0) {
			fprintf(stderr, "The length of the derived password "
					"can't be negative or zero.\n");
			return EINVAL;
		}
		options.dkLen = tmp;
		break;
	case 'n':
		options.passno = arg;
		break;

	case ARGP_KEY_ARG:
		switch (state->arg_num) {
		case 0:
			options.domain = arg;
			break;
		case 1:
			options.username = arg;
			break;
		default:
			fputs("error: too many arguments\n", stderr);
			argp_usage(state);
			return EINVAL;
		}
		break;

	case ARGP_KEY_END:
		if (state->arg_num < 2) {
			fputs("error: not enough arguments\n", stderr);
			argp_usage(state);
		}
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = {
		argp_options, &parse_opt,
		"<domain> <username>",
		"Generates a deterministic password from the <domain> and "
		"<username>. Optionally a password iteration number may be "
		"given to generate new passwords."
};

#ifndef TEST
int main(int argc, char *argv[])
{
	int ret;
	char *master_pwd;
	size_t master_pwd_len;
	char *salt;
	size_t salt_len;
	uint8_t *buf;

	ret = argp_parse(&argp, argc, argv, 0, 0, &options);
	if (ret != 0)
		exit(1);

	if (options.passno == NULL) {
		options.passno = "0";
	}

	if (options.dkLen == 0) {
		options.dkLen = 64;
	}

	if (options.chars == NULL) {
		options.chars = "";
	}

	salt_len = strlen(options.domain) + strlen(options.username)
			+ strlen(options.passno);
	salt = malloc(salt_len + 1);
	if (salt == NULL) {
		perror("Could not allocate memory for the salt");
		exit(1);
	}
	memcpy(salt, options.domain, strlen(options.domain) + 1);
	memcpy(salt + strlen(salt), options.username, strlen(options.username) + 1);
	memcpy(salt + strlen(salt), options.passno, strlen(options.passno) + 1);

	/*
	 * Ask the user for his master password. The length of the input buffer
	 * is fixed to 64 bytes, since no human being that needs a password
	 * derivator can memorize a password longer than 63 characters.
	 */
	master_pwd = malloc(64);
	if (master_pwd == NULL) {
		perror("Could not allocate memory for the master password");
		exit(1);
	}
	master_pwd_len = 64;
	ret = ask_password(master_pwd, &master_pwd_len);
	if (ret != 0) {
		perror("While reading the master password from `stdin`");
		goto fail;
	}

	buf = malloc(options.dkLen + 1);
	if (buf == NULL) {
		perror("Could not allocate memory for the domain password");
		goto fail;
	}
	ret = crypto_scrypt((uint8_t *)master_pwd, master_pwd_len,
			(uint8_t *)salt, salt_len, MP_N, MP_r, MP_p, buf,
			options.dkLen);
	memset(master_pwd, 0, 64);
	master_pwd_len = 0;
	if (ret != 0) {
		perror("While deriving the domain password");
		goto fail;
	}

	char *chars;
	size_t len;
	ret = enumerate_charset(options.chars, &chars, &len);
	if (ret != 0) {
		perror("While enumerating the charset");
		goto fail;
	}
	to_pwdchars(buf, options.dkLen, chars, len);
	fprintf(stdout, "%s\n", buf);

	exit(0);

fail:
	memset(master_pwd, 0, 64);
	master_pwd_len = 0;
	exit(1);
}
#endif

static char *to_pwdchars(uint8_t *str, size_t len, char *chars, size_t clen)
{
	for (; len > 0; len--) {
		*str = chars[*str % clen];
		str++;
	}
	*str = '\0';

	return (char *)str;
}

static int enumerate_charset(const char *def, char **res, size_t *rlen)
{
	char chrs[95];	/* as big as `|*|` plus one for the \0 */
	char l;		/* left side of a character range */
	int op;		/* operator found (`-`) <- not a smiley! */
	char *result;
	size_t len;

	if (def == NULL || res == NULL || rlen == NULL) {
		errno = EINVAL;
		return -1;
	}

	result = chrs;
	len = strlen(def);

	/* Resolve character classes.  If no `def` is given, assume all ASCII
	 * characters may be used.
	 */
	if (len == 0 || strcmp(def, ":graph:") == 0 || strcmp(def, "*") == 0) {
		def = "!-~";
	}
	else if (strcmp(def, ":alnum:") == 0) {
		def = "a-zA-Z0-9";
	}
	else if (strcmp(def, ":alpha:") == 0) {
		def = "a-zA-Z";
	}
	else if (strcmp(def, ":digit:") == 0) {
		def = "0-9";
	}
	else if (strcmp(def, ":lower:") == 0) {
		def = "a-z";
	}
	else if (strcmp(def, ":punct:") == 0) {
		def = "!-/:-@[-`{-~";
	}
	else if (strcmp(def, ":upper:") == 0) {
		def = "A-Z";
	}
	else if (strcmp(def, ":word:") == 0) {
		def = "A-Za-z0-9_";
	}
	else if (strcmp(def, ":xdigit:") == 0) {
		def = "A-Fa-f0-9";
	}

	l = '\0';
	op = 0;
	while (*def != '\0') {
		register char c = *def;
		if (isspace(c))
			continue;

		if (l == '\0' && c == '-') {
			*result = c;
			result++;
		}
		else if (l == '\0' && c != '-') {
			l = c;
		}
		else if (l != '\0' && c == '-') {
			op = 1;
		}
		else if (l != '\0' && c != '-' && op == 1) {
			for (; l <= c; l++) {
				*result = l;
				result++;
			}
			op = 0;
			l = '\0';
		}
		else if (l != '\0' && c != '-' && op == 0) {
			*result = l;
			result++;
			l = c;
		}

		def++;
	}

	if (l != '\0') {
		*result = l;
		result++;
	}
	if (op == 1) {
		*result = '-';
		result++;
	}
	*result = '\0';

	*rlen = strlen(chrs);
	*res = malloc(*rlen);
	if (*res == NULL) {
		perror("enumerate_charset()");
		return -1;
	}
	memcpy(*res, chrs, *rlen + 1);

	return 0;
}

static int ask_password(char *passwd, size_t *len)
{
	static struct termios term_noecho, term_default;
	int c;
	size_t curr_len;

	/* turn off echoing */
	tcgetattr(STDIN_FILENO, &term_default);
	term_noecho = term_default;
	term_noecho.c_lflag &= ~(ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term_noecho);

	fprintf(stdout, "Enter your master password: ");
	fflush(stdout);
	curr_len = 0;
	while ((c = fgetc(stdin)) != EOF && c != '\n' && curr_len < *len) {
		passwd[curr_len] = c;
		curr_len++;
	}
	passwd[curr_len] = 0;

	*len = curr_len;

	/* turn echoing back on */
	tcsetattr(STDIN_FILENO, TCSANOW, &term_default);
	fprintf(stdout, "\n");

	if (c == EOF)
		return -1;
	return 0;
}
