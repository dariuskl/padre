//
//   Copyright 2024 Darius Kellermann
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

#ifndef PADRE_H_INCLUDED
#define PADRE_H_INCLUDED

#if __STDC_VERSION__ < 202300L
#define nullptr ((void *)0)
#endif

// The length of the input buffer is fixed to 64 bytes because I assume that
// anyone that can memorize a longer password does not need this utility.
#define MAX_MASTER_PASSWORD_LENGTH 64

#define MAX_DATABASE_FILE_SIZE (1024 * 16)
#define AVERAGE_DATABASE_ENTRY_SIZE 60

// These settings correspond with the defaults of the Python scrypt bindings.
// ... for historical reasons ...
#define MP_N 16384
#define MP_r 8
#define MP_p 1

#endif // PADRE_H_INCLUDED
