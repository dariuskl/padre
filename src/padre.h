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

/** Configuration and global data structures.
 * @file	padre.h
 * @author	Darius Kellermann <darius.kellermann@gmail.com>
 */

#include <stddef.h>

/**
 * Provides access to all command-line arguments that were parsed.
 */
struct cli_opts {
	const char *domain;
	const char *username;
	const char *passno;
	const char *chars;
	size_t dkLen;
} options = {NULL, NULL, NULL, NULL, 0};

#define MP_N		32768
#define MP_r		8
#define MP_p		1
