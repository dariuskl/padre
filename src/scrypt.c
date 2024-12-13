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

#include "sha256.c"
#include "nonstd.h"

// stuff from lib/crypto etc.

static void (*smix_func)(u8 *, usize, u64, void *, void *) = 0;

static void blkcpy(u32 *dest, const u32 *src, usize len) {
  copy_b(src, (const byte *)src + len, dest, (const byte *)dest + len);
}

static void blkxor(u32 *dest, const u32 *src, usize len) {
  usize i;

  for (i = 0; i < len / 4; i++)
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
static void blockmix_salsa8(const u32 *Bin, u32 *Bout, u32 *X, usize r) {
  usize i;

  /* 1: X <-- B_{2r - 1} */
  blkcpy(X, &Bin[(2 * r - 1) * 16], 64);

  /* 2: for i = 0 to 2r - 1 do */
  for (i = 0; i < 2 * r; i += 2) {
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
static u64 integerify(const u32 *B, usize r) {
  const u32 *X = B + (2 * r - 1) * 16;

  return (((u64)(X[1]) << 32) + X[0]);
}

static inline void le32enc(void *pp, u32 x) {
  u8 *p = (u8*)pp;

  p[0] = x & 0xff;
  p[1] = (x >> 8) & 0xff;
  p[2] = (x >> 16) & 0xff;
  p[3] = (u8)((x >> 24) & 0xff);
}

static inline u32 le32dec(const void *pp) {
  const u8 *p = (u8 const *)pp;

  return ((u32)(p[0]) | ((u32)(p[1]) << 8) |
    ((u32)(p[2]) << 16) | ((u32)(p[3]) << 24));
}

/**
 * crypto_scrypt_smix(B, r, N, V, XY):
 * Compute B = SMix_r(B, N).  The input B must be 128r bytes in length;
 * the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r + 64 bytes in length.  The value N must be a
 * power of 2 greater than 1.  The arrays B, V, and XY must be aligned to a
 * multiple of 64 bytes.
 */
void crypto_scrypt_smix(u8 *B, usize r, u64 N, void *_v, void *XY) {
  u32 *X = XY;
  u32 *Y = (void *)((u8 *)(XY) + 128 * r);
  u32 *Z = (void *)((u8 *)(XY) + 256 * r);
  u32 *V = _v;
  u64 i;
  u64 j;
  usize k;

  /* 1: X <-- B */
  for (k = 0; k < 32 * r; k++)
    X[k] = le32dec(&B[4 * k]);

  /* 2: for i = 0 to N - 1 do */
  for (i = 0; i < N; i += 2) {
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
  for (i = 0; i < N; i += 2) {
    /* 7: j <-- Integerify(X) mod N */
    j = integerify(X, r) & (N - 1);

    /* 8: X <-- H(X \xor V_j) */
    blkxor(X, &V[j * (32 * r)], 128 * r);
    blockmix_salsa8(X, Y, Z, r);

    /* 7: j <-- Integerify(X) mod N */
    j = integerify(Y, r) & (N - 1);

    /* 8: X <-- H(X \xor V_j) */
    blkxor(Y, &V[j * (32 * r)], 128 * r);
    blockmix_salsa8(Y, X, Z, r);
  }

  /* 10: B' <-- X */
  for (k = 0; k < 32 * r; k++)
    le32enc(&B[4 * k], X[k]);
}

/**
 * crypto_scrypt_internal(passwd, passwdlen, salt, saltlen, N, r, p, buf,
 *     buflen, smix):
 * Perform the requested scrypt computation, using ${smix} as the smix routine.
 */
static int crypto_scrypt_internal(arena *a, const u8 *passwd, usize passwdlen,
                                  const u8 *salt, usize saltlen,
                                  u64 N, u32 _r, u32 _p,
                                  u8 *buf, usize buflen,
                                  void (*smix)(u8 *, usize, u64, void *, void *)) {
  void *B0, *V0, *XY0;
  u8 *B;
  u32 *V;
  u32 *XY;
  usize r = _r, p = _p;
  u32 i;

  /* Sanity-check parameters. */
  if ((r == 0) || (p == 0)) {
    return -1;
  }
#if __SIZE_MAX__ > __UINT32_MAX__
  if (buflen > (((u64)(1) << 32) - 1) * 32) {
    return -1;
  }
#endif
  if ((u64)(r) * (u64)(p) >= (1 << 30)) {
    return -1;
  }
  if (((N & (N - 1)) != 0) || (N < 2)) {
    return -1;
  }
  if ((r > __SIZE_MAX__ / 128 / p) ||
#if __SIZE_MAX__ / 256 <= __UINT32_MAX__
	    (r > (__SIZE_MAX__ - 64) / 256) ||
#endif
    (N > __SIZE_MAX__ / 128 / r)) {
    return -1;
  }

  /* Allocate memory. */
  if ((B0 = arena_try_push(a, (size)(128 * r * p + 63)).begin) == 0) // TODO conversion
    return -1;
  B = (u8*)(((uptr)(B0) + 63) & ~(uptr)(63));
  if ((XY0 = arena_try_push(a, (size)(256 * r + 64 + 63)).begin) == 0) // TODO conversion
    return -1;
  XY = (u32*)(((uptr)(XY0) + 63) & ~(uptr)(63));
  if ((V0 = arena_try_push(a, (size)(128 * r * N + 63)).begin) == 0) // TODO conversion
    return -1;
  V = (u32*)(((uptr)(V0) + 63) & ~(uptr)(63));

  /* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
  buf8 tbuf = (buf8){B, B, B + p * 128 * r};
  pbkdf2_sha256((utf8){passwd, passwd + passwdlen}, (view8){salt, salt + saltlen}, 1, &tbuf);

  /* 2: for i = 0 to p - 1 do */
  for (i = 0; i < p; i++) {
    /* 3: B_i <-- MF(B_i, N) */
    smix(&B[i * 128 * r], r, N, V, XY);
  }

  /* 5: DK <-- PBKDF2(P, B, 1, dkLen) */
  tbuf = (buf8){buf, buf, buf + buflen};
  pbkdf2_sha256((utf8){passwd, passwd + passwdlen}, (view8){B, B + p * 128 * r}, 1, &tbuf);

  // TODO consider popping the memory from the arena again

  return 0;
}

#define TESTLEN 64

static struct scrypt_test {
  const char *passwd;
  const char *salt;
  u64 N;
  u32 r;
  u32 p;
  u8 result[TESTLEN];
} testcase = {
  .passwd = "pleaseletmein",
  .salt = "SodiumChloride",
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

static int testsmix(arena *a, void (*smix)(u8 *, usize, u64, void *, void *)) {
  u8 hbuf[TESTLEN];

  // Perform the computation.
  if (crypto_scrypt_internal(a,
                             (const u8*)testcase.passwd, (usize)ascii_length_of(testcase.passwd), // TODO conversion
                             (const u8*)testcase.salt, (usize)ascii_length_of(testcase.salt), // TODO conversion
                             testcase.N, testcase.r, testcase.p, hbuf, TESTLEN, smix))
    return (-1);

  // Does it match?
  return equal_b_n(testcase.result, hbuf, TESTLEN) ? 0 : 1;
}

/**
 * Compute scrypt(passwd[0 ... passwdlen - 1], salt[0 ... saltlen - 1], N, r,
 * p, buflen) and write the result into buf.  The parameters r, p, and buflen
 * must satisfy 0 < r * p < 2^30 and buflen <= (2^32 - 1) * 32.  The parameter
 * N must be a power of 2 greater than 1.
 *
 * Return 0 on success; or -1 on error.
 */
int scrypt(arena *a, const u8 *passwd, size passwdlen,
           const u8 *salt, size saltlen,
           u64 N, u32 _r, u32 _p,
           buf8 *password) {
  // Ensure generic smix works.
  if (!testsmix(a, crypto_scrypt_smix)) {
    smix_func = crypto_scrypt_smix;
    return crypto_scrypt_internal(a, passwd, (usize)passwdlen,
                                  salt, (usize)saltlen, N, _r, _p,
                                  password->eod,
                                  (usize)buf8_capacity(*password),
                                  smix_func);
  }

  print(utf8("error: cannot derive passwords - scrypt smix failed test"));
  return -1;
}
