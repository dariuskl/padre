// This is free and unencumbered software released into the public domain.
// EXCEPT ANYTHING COPIED FROM scrypt WHICH IS FOUND BELOW THE ORIGINAL
// LICENSE.

#include "sha256.c"

static inline void put_unaligned_le_u32(byte vec[static 4], const u32 val) {
  vec[0] = (byte) val       ;
  vec[1] = (byte)(val >>  8);
  vec[2] = (byte)(val >> 16);
  vec[3] = (byte)(val >> 24);
}

static inline u32 get_unaligned_le_u32(const byte vec[static 4]) {
  return vec[0] | vec[1] << 8 | vec[2] << 16 | vec[3] << 24;
}

// stuff from lib/crypto etc.
/*-
 * Copyright 2009 Colin Percival
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

static void blkcpy(u32 *dest, const u32 *src, size len) {
  copy_b(src, (const byte *)src + len, dest, (const byte *)dest + len);
}

static void blkxor(u32 *dest, const u32 *src, size len) {
  for (size i = 0; i < len / 4; i++)
    dest[i] ^= src[i];
}

/**
 * salsa20_8(B):
 * Apply the salsa20/8 core to the provided block.
 */
static void salsa20_8(u32 B[16]) {
  u32 x[16];
  usize i;

  blkcpy(x, B, 64);
  for (i = 0; i < 8; i += 2) {
#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
    /* Operate on columns. */
    x[4] ^= R(x[ 0]+x[12], 7);
    x[8] ^= R(x[ 4]+x[ 0], 9);
    x[12] ^= R(x[ 8]+x[ 4], 13);
    x[0] ^= R(x[12]+x[ 8], 18);

    x[9] ^= R(x[ 5]+x[ 1], 7);
    x[13] ^= R(x[ 9]+x[ 5], 9);
    x[1] ^= R(x[13]+x[ 9], 13);
    x[5] ^= R(x[ 1]+x[13], 18);

    x[14] ^= R(x[10]+x[ 6], 7);
    x[2] ^= R(x[14]+x[10], 9);
    x[6] ^= R(x[ 2]+x[14], 13);
    x[10] ^= R(x[ 6]+x[ 2], 18);

    x[3] ^= R(x[15]+x[11], 7);
    x[7] ^= R(x[ 3]+x[15], 9);
    x[11] ^= R(x[ 7]+x[ 3], 13);
    x[15] ^= R(x[11]+x[ 7], 18);

    /* Operate on rows. */
    x[1] ^= R(x[ 0]+x[ 3], 7);
    x[2] ^= R(x[ 1]+x[ 0], 9);
    x[3] ^= R(x[ 2]+x[ 1], 13);
    x[0] ^= R(x[ 3]+x[ 2], 18);

    x[6] ^= R(x[ 5]+x[ 4], 7);
    x[7] ^= R(x[ 6]+x[ 5], 9);
    x[4] ^= R(x[ 7]+x[ 6], 13);
    x[5] ^= R(x[ 4]+x[ 7], 18);

    x[11] ^= R(x[10]+x[ 9], 7);
    x[8] ^= R(x[11]+x[10], 9);
    x[9] ^= R(x[ 8]+x[11], 13);
    x[10] ^= R(x[ 9]+x[ 8], 18);

    x[12] ^= R(x[15]+x[14], 7);
    x[13] ^= R(x[12]+x[15], 9);
    x[14] ^= R(x[13]+x[12], 13);
    x[15] ^= R(x[14]+x[13], 18);
#undef R
  }
  for (i = 0; i < 16; i++)
    B[i] += x[i];
}

/**
 * blockmix_salsa8(Bin, Bout, X, r):
 * Compute Bout = BlockMix_{salsa20/8, r}(Bin).  The input Bin must be 128r
 * bytes in length; the output Bout must also be the same size.  The
 * temporary space X must be 64 bytes.
 */
static void blockmix_salsa8(const u32 *Bin, u32 *Bout, u32 *X, int r) {
  /* 1: X <-- B_{2r - 1} */
  blkcpy(X, &Bin[(2 * r - 1) * 16], 64);

  /* 2: for i = 0 to 2r - 1 do */
  for (size i = 0; i < 2 * r; i += 2) {
    /* 3: X <-- H(X \xor B_i) */
    blkxor(X, &Bin[i * 16], 64);
    salsa20_8(X);

    /* 4: Y_i <-- X */
    /* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
    blkcpy(&Bout[i * 8], X, 64);

    /* 3: X <-- H(X \xor B_i) */
    blkxor(X, &Bin[i * 16 + 16], 64);
    salsa20_8(X);

    /* 4: Y_i <-- X */
    /* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
    blkcpy(&Bout[i * 8 + r * 16], X, 64);
  }
}

/**
 * integerify(B, r):
 * Return the result of parsing B_{2r-1} as a little-endian integer.
 */
static u64 integerify(const u32 *B, int r) {
  const u32 *X = B + (2 * r - 1) * 16;

  return (((u64)(X[1]) << 32) + X[0]);
}

/**
 * crypto_scrypt_smix(B, r, N, V, XY):
 * Compute B = SMix_r(B, N).  The input B must be 128r bytes in length;
 * the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r + 64 bytes in length.  The value N must be a
 * power of 2 greater than 1.  The arrays B, V, and XY must be aligned to a
 * multiple of 64 bytes.
 */
void crypto_scrypt_smix(u8 *B, int r, i64 N, void *_v, void *XY) {
  u32 *X = XY;
  u32 *Y = (void *)((u8 *)XY + 128 * r);
  u32 *Z = (void *)((u8 *)XY + 256 * r);
  u32 *V = _v;

  /* 1: X <-- B */
  for (size k = 0; k < 32 * r; ++k)
    X[k] = get_unaligned_le_u32(&B[4 * k]);

  /* 2: for i = 0 to N - 1 do */
  for (i64 i = 0; i < N; i += 2) {
    /* 3: V_i <-- X */
    blkcpy(&V[i * (32 * r)], X, 128 * r);

    /* 4: X <-- H(X) */
    blockmix_salsa8(X, Y, Z, r);

    /* 3: V_i <-- X */
    blkcpy(&V[(i + 1) * (32 * r)], Y, 128 * r);

    /* 4: X <-- H(X) */
    blockmix_salsa8(Y, X, Z, r);
  }

  /* 6: for i = 0 to N - 1 do */
  for (i64 i = 0; i < N; i += 2) {
    /* 7: j <-- Integerify(X) mod N */
    u64 j = integerify(X, r) & (u64)(N - 1);

    /* 8: X <-- H(X \xor V_j) */
    blkxor(X, &V[j * (u64)(32 * r)], 128 * r);
    blockmix_salsa8(X, Y, Z, r);

    /* 7: j <-- Integerify(X) mod N */
    j = integerify(Y, r) & (u64)(N - 1);

    /* 8: X <-- H(X \xor V_j) */
    blkxor(Y, &V[j * (u64)(32 * r)], 128 * r);
    blockmix_salsa8(Y, X, Z, r);
  }

  /* 10: B' <-- X */
  for (size k = 0; k < 32 * r; ++k)
    put_unaligned_le_u32(&B[4 * k], X[k]);
}

static int crypto_scrypt_internal(arena *a, utf8 passwd, view8 salt,
                                  i64 N, int r, int p,
                                  u8 *buf, size buflen) {
  void *B0, *V0, *XY0;
  u8 *B;
  u32 *V;
  u32 *XY;

  /* Sanity-check parameters. */
  if (N <= 0 || r <= 0 || p <= 0) {
    return -1;
  }
#if __PTRDIFF_MAX__ > __INT32_MAX__
  if (buflen > (((i64)1 << 32) - 1) * 32) {
    return -1;
  }
#endif
  if ((i64)r * (i64)p >= 1 << 30) {
    return -1;
  }
  if (((N & (N - 1)) != 0) || (N < 2)) {
    return -1;
  }
  if ((r > __PTRDIFF_MAX__ / 128 / p)
#if __PTRDIFF_MAX__ / 256 <= __INT32_MAX__
      || (r > (__PTRDIFF_MAX__ - 64) / 256)
#endif
      || (N > __PTRDIFF_MAX__ / 128 / r)) {
    return -1;
  }

  /* Allocate memory. */
  if ((B0 = arena_try_push(a, 128 * r * p + 63).begin) == 0)
    return -1;
  B = (u8*)(((uptr)(B0) + 63) & ~(uptr)(63));
  if ((XY0 = arena_try_push(a, 256 * r + 64 + 63).begin) == 0)
    return -1;
  XY = (u32*)(((uptr)(XY0) + 63) & ~(uptr)(63));
  if ((V0 = arena_try_push(a, 128 * r * N + 63).begin) == 0)
    return -1;
  V = (u32*)(((uptr)(V0) + 63) & ~(uptr)(63));

  /* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
  buf8 tbuf = (buf8){B, B, B + p * 128 * r};
  pbkdf2_sha256(passwd, salt, 1, &tbuf);

  /* 2: for i = 0 to p - 1 do */
  for (int i = 0; i < p; ++i) {
    /* 3: B_i <-- MF(B_i, N) */
    crypto_scrypt_smix(&B[i * 128 * r], r, N, V, XY);
  }

  /* 5: DK <-- PBKDF2(P, B, 1, dkLen) */
  tbuf = (buf8){buf, buf, buf + buflen};
  pbkdf2_sha256(passwd, (view8){B, B + p * 128 * r}, 1, &tbuf);

  return 0;
}

#define TESTLEN 64

static struct scrypt_test {
  const u8 *passwd;
  const u8 *salt;
  i64 N;
  int r;
  int p;
  u8 result[TESTLEN];
} testcase = {
  .passwd = u8"pleaseletmein",
  .salt = u8"SodiumChloride",
  .N = 16,
  .r = 8,
  .p = 1,
  .result = {
    0x25, 0xa9, 0xfa, 0x20, 0x7f, 0x87, 0xca, 0x09,
    0xa4, 0xef, 0x8b, 0x9f, 0x77, 0x7a, 0xca, 0x16,
    0xbe, 0xb7, 0x84, 0xae, 0x18, 0x30, 0xbf, 0xbf,
    0xd3, 0x83, 0x25, 0xaa, 0xbb, 0x93, 0x77, 0xdf,
    0x1b, 0xa7, 0x84, 0xd7, 0x46, 0xea, 0x27, 0x3b,
    0xf5, 0x16, 0xa4, 0x6f, 0xbf, 0xac, 0xf5, 0x11,
    0xc5, 0xbe, 0xba, 0x4c, 0x4a, 0xb3, 0xac, 0xc7,
    0xfa, 0x6f, 0x46, 0x0b, 0x6c, 0x0f, 0x47, 0x7b,
  }
};

static bool test_smix(arena *a) {
  u8 hbuf[TESTLEN];

  // Perform the computation.
  if (crypto_scrypt_internal(a, to_utf8(testcase.passwd),
                             utf8_to_view(to_utf8(testcase.salt)),
                             testcase.N, testcase.r, testcase.p, hbuf,
                             TESTLEN))
    return false;

  // Does it match?
  return equal_b_n(testcase.result, hbuf, TESTLEN);
}

// Computes `scrypt(passwd, salt, N, r, p, len)` with `len` being the available
// bytes in the buffer `derived`, and writes the result into `derived`.
//
// The parameters r, p, and len must satisfy
//    0 < r * p < 2^30
// and
//    len <= (2^32 - 1) * 32.
// The parameter N must be a power of 2 greater than 1.
//
// Returns 0 on success; -1 on error.
int scrypt(arena *a, utf8 passwd, view8 salt, i64 N, int r, int p,
           buf8 *derived) {
  if (!test_smix(a)) {
    println("error: cannot derive passwords - scrypt smix failed test");
    return -1;
  }
  return crypto_scrypt_internal(a, passwd, salt, N, r, p, derived->eod,
                                buf8_available(*derived));
}
