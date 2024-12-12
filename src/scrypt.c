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

// stuff from libcperciva/alg/sha256.[ch]

static inline u32
be32dec(const void * pp)
{
	const u8 * p = (u8 const *)pp;

	return ((u32)(p[3]) | ((u32)(p[2]) << 8) |
		((u32)(p[1]) << 16) | ((u32)(p[0]) << 24));
}

/*
 * Decode a big-endian length len vector of (u8) into a length
 * len/4 vector of (u32).  Assumes len is a multiple of 4.
 */
static void
be32dec_vect(u32 * dst, const u8 * src, size len)
{
	size i;

	/* Sanity-check. */
	assert(len % 4 == 0);

	/* Decode vector, one word at a time. */
	for (i = 0; i < len / 4; i++)
		dst[i] = be32dec(src + i * 4);
}

static inline void
be32enc(void * pp, u32 x)
{
	u8 * p = (u8 *)pp;

	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (u8)((x >> 24) & 0xff);
}

/*
 * Encode a length len/4 vector of (u32) into a length len vector of
 * (u8) in big-endian form.  Assumes len is a multiple of 4.
 */
static void
be32enc_vect(u8 * dst, const u32 * src, usize len)
{
	usize i;

	/* Sanity-check. */
	assert(len % 4 == 0);

	/* Encode vector, one word at a time. */
	for (i = 0; i < len / 4; i++)
		be32enc(dst + i * 4, src[i]);
}

/* Context structure for SHA256 operations. */
typedef struct {
	u32 state[8];
	u64 count;
	u8 buf[64];
} SHA256_CTX;

/* Elementary functions used by SHA256 */
#define Ch(x, y, z)	((x & (y ^ z)) ^ z)
#define Maj(x, y, z)	((x & (y | z)) | (y & z))
#define SHR(x, n)	(x >> n)
#define ROTR(x, n)	((x >> n) | (x << (32 - n)))
#define S0(x)		(ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S1(x)		(ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define s0(x)		(ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define s1(x)		(ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

/* SHA256 round function */
#define RND(a, b, c, d, e, f, g, h, k)			\
h += S1(e) + Ch(e, f, g) + k;			\
d += h;						\
h += S0(a) + Maj(a, b, c)

/* SHA256 round constants. */
static const u32 Krnd[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Adjusted round function for rotating state */
#define RNDr(S, W, i, ii)			\
RND(S[(64 - i) % 8], S[(65 - i) % 8],	\
S[(66 - i) % 8], S[(67 - i) % 8],	\
S[(68 - i) % 8], S[(69 - i) % 8],	\
S[(70 - i) % 8], S[(71 - i) % 8],	\
W[i + ii] + Krnd[i + ii])

/* Message schedule computation */
#define MSCH(W, ii, i)				\
W[i + ii + 16] = s1(W[i + ii + 14]) + W[i + ii + 9] + s0(W[i + ii + 1]) + W[i + ii]

/*
 * SHA256 block compression function.  The 256-bit state is transformed via
 * the 512-bit input block to produce a new state.  The arrays W and S may be
 * filled with sensitive data, and should be sanitized by the callee.
 */
static void
SHA256_Transform(u32 state[static restrict 8],
    const u8 block[static restrict 64],
    u32 W[static restrict 64], u32 S[static restrict 8])
{
	int i;

	/* 1. Prepare the first part of the message schedule W. */
	be32dec_vect(W, block, 64);

	/* 2. Initialize working variables. */
	memcpy(S, state, 32);

	/* 3. Mix. */
	for (i = 0; i < 64; i += 16) {
		RNDr(S, W, 0, i);
		RNDr(S, W, 1, i);
		RNDr(S, W, 2, i);
		RNDr(S, W, 3, i);
		RNDr(S, W, 4, i);
		RNDr(S, W, 5, i);
		RNDr(S, W, 6, i);
		RNDr(S, W, 7, i);
		RNDr(S, W, 8, i);
		RNDr(S, W, 9, i);
		RNDr(S, W, 10, i);
		RNDr(S, W, 11, i);
		RNDr(S, W, 12, i);
		RNDr(S, W, 13, i);
		RNDr(S, W, 14, i);
		RNDr(S, W, 15, i);

		if (i == 48)
			break;
		MSCH(W, 0, i);
		MSCH(W, 1, i);
		MSCH(W, 2, i);
		MSCH(W, 3, i);
		MSCH(W, 4, i);
		MSCH(W, 5, i);
		MSCH(W, 6, i);
		MSCH(W, 7, i);
		MSCH(W, 8, i);
		MSCH(W, 9, i);
		MSCH(W, 10, i);
		MSCH(W, 11, i);
		MSCH(W, 12, i);
		MSCH(W, 13, i);
		MSCH(W, 14, i);
		MSCH(W, 15, i);
	}

	/* 4. Mix local working variables into global state. */
	for (i = 0; i < 8; i++)
		state[i] += S[i];
}

static const u8 PAD[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static inline void
be64enc(void * pp, u64 x)
{
	u8 * p = (u8 *)pp;

	p[7] = x & 0xff;
	p[6] = (x >> 8) & 0xff;
	p[5] = (x >> 16) & 0xff;
	p[4] = (x >> 24) & 0xff;
	p[3] = (x >> 32) & 0xff;
	p[2] = (x >> 40) & 0xff;
	p[1] = (x >> 48) & 0xff;
	p[0] = (u8)(x >> 56) & 0xff;
}

/* Add padding and terminating bit-count. */
static void
SHA256_Pad(SHA256_CTX * ctx, u32 tmp32[static restrict 72])
{
	usize r;

	/* Figure out how many bytes we have buffered. */
	r = (ctx->count >> 3) & 0x3f;

	/* Pad to 56 mod 64, transforming if we finish a block en route. */
	if (r < 56) {
		/* Pad to 56 mod 64. */
		memcpy(&ctx->buf[r], PAD, 56 - r);
	} else {
		/* Finish the current block and mix. */
		memcpy(&ctx->buf[r], PAD, 64 - r);
		SHA256_Transform(ctx->state, ctx->buf, &tmp32[0], &tmp32[64]);

		/* The start of the final block is all zeroes. */
		memset(&ctx->buf[0], 0, 56);
	}

	/* Add the terminating bit-count. */
	be64enc(&ctx->buf[56], ctx->count);

	/* Mix in the final block. */
	SHA256_Transform(ctx->state, ctx->buf, &tmp32[0], &tmp32[64]);
}

/* Magic initialization constants. */
static const u32 initial_state[8] = {
	0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
	0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/**
 * SHA256_Init(ctx):
 * Initialize the SHA256 context ${ctx}.
 */
void
SHA256_Init(SHA256_CTX * ctx)
{
	/* Zero bits processed so far. */
	ctx->count = 0;

	/* Initialize state. */
	memcpy(ctx->state, initial_state, sizeof(initial_state));
}

/**
 * SHA256_Update(ctx, in, len):
 * Input ${len} bytes from ${in} into the SHA256 context ${ctx}.
 */
static void
SHA256_Update_internal(SHA256_CTX * ctx, const void * in, usize len,
	u32 tmp32[static restrict 72])
{
	u32 r;
	const u8 * src = in;

	/* Return immediately if we have nothing to do. */
	if (len == 0)
		return;

	/* Number of bytes left in the buffer from previous updates. */
	r = (ctx->count >> 3) & 0x3f;

	/* Update number of bits. */
	ctx->count += (u64)(len) << 3;

	/* Handle the case where we don't need to perform any transforms. */
	if (len < 64 - r) {
		memcpy(&ctx->buf[r], src, len);
		return;
	}

	/* Finish the current block. */
	memcpy(&ctx->buf[r], src, 64 - r);
	SHA256_Transform(ctx->state, ctx->buf, &tmp32[0], &tmp32[64]);
	src += 64 - r;
	len -= 64 - r;

	/* Perform complete blocks. */
	while (len >= 64) {
		SHA256_Transform(ctx->state, src, &tmp32[0], &tmp32[64]);
		src += 64;
		len -= 64;
	}

	/* Copy left over data into buffer. */
	memcpy(ctx->buf, src, len);
}

/**
 * SHA256_Final(digest, ctx):
 * Output the SHA256 hash of the data input to the context ${ctx} into the
 * buffer ${digest}, and clear the context state.
 */
static void
SHA256_Final_internal(u8 digest[32], SHA256_CTX * ctx,
	u32 tmp32[static restrict 72])
{

	/* Add padding. */
	SHA256_Pad(ctx, tmp32);

	/* Write the hash. */
	be32enc_vect(digest, ctx->state, 32);
}

/* Context structure for HMAC-SHA256 operations. */
typedef struct {
	SHA256_CTX ictx;
	SHA256_CTX octx;
} HMAC_SHA256_CTX;

/**
 * HMAC_SHA256_Init(ctx, K, Klen):
 * Initialize the HMAC-SHA256 context ${ctx} with ${Klen} bytes of key from
 * ${K}.
 */
static void
HMAC_SHA256_Init_internal(HMAC_SHA256_CTX * ctx, const void * _k, usize Klen,
	u32 tmp32[static restrict 72], u8 pad[static restrict 64],
	u8 khash[static restrict 32])
{
	const u8 * K = _k;
	usize i;

	/* If Klen > 64, the key is really SHA256(K). */
	if (Klen > 64) {
		SHA256_Init(&ctx->ictx);
		SHA256_Update_internal(&ctx->ictx, K, Klen, tmp32);
		SHA256_Final_internal(khash, &ctx->ictx, tmp32);
		K = khash;
		Klen = 32;
	}

	/* Inner SHA256 operation is SHA256(K xor [block of 0x36] || data). */
	SHA256_Init(&ctx->ictx);
	memset(pad, 0x36, 64);
	for (i = 0; i < Klen; i++)
		pad[i] ^= K[i];
	SHA256_Update_internal(&ctx->ictx, pad, 64, tmp32);

	/* Outer SHA256 operation is SHA256(K xor [block of 0x5c] || hash). */
	SHA256_Init(&ctx->octx);
	memset(pad, 0x5c, 64);
	for (i = 0; i < Klen; i++)
		pad[i] ^= K[i];
	SHA256_Update_internal(&ctx->octx, pad, 64, tmp32);
}

/**
 * HMAC_SHA256_Update(ctx, in, len):
 * Input ${len} bytes from ${in} into the HMAC-SHA256 context ${ctx}.
 */
static void
HMAC_SHA256_Update_internal(HMAC_SHA256_CTX * ctx, const void * in, usize len,
	u32 tmp32[static restrict 72])
{

	/* Feed data to the inner SHA256 operation. */
	SHA256_Update_internal(&ctx->ictx, in, len, tmp32);
}

/**
 * HMAC_SHA256_Final(digest, ctx):
 * Output the HMAC-SHA256 of the data input to the context ${ctx} into the
 * buffer ${digest}, and clear the context state.
 */
static void
HMAC_SHA256_Final_internal(u8 digest[32], HMAC_SHA256_CTX * ctx,
	u32 tmp32[static restrict 72], u8 ihash[static restrict 32])
{
	/* Finish the inner SHA256 operation. */
	SHA256_Final_internal(ihash, &ctx->ictx, tmp32);

	/* Feed the inner hash to the outer SHA256 operation. */
	SHA256_Update_internal(&ctx->octx, ihash, 32, tmp32);

	/* Finish the outer SHA256 operation. */
	SHA256_Final_internal(digest, &ctx->octx, tmp32);
}

/**
 * PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, c, buf, dkLen):
 * Compute PBKDF2(passwd, salt, c, dkLen) using HMAC-SHA256 as the PRF, and
 * write the output to buf.  The value dkLen must be at most 32 * (2^32 - 1).
 */
void
PBKDF2_SHA256(const u8 * passwd, usize passwdlen, const u8 * salt,
    usize saltlen, u64 c, u8 * buf, usize dkLen)
{
	HMAC_SHA256_CTX Phctx, PShctx, hctx;
	u32 tmp32[72];
	u8 tmp8[96];
	usize i;
	u8 ivec[4];
	u8 U[32];
	u8 T[32];
	u64 j;
	int k;
	usize clen;

#if SIZE_MAX >= (32 * UINT32_MAX)
	/* Sanity-check. */
	assert(dkLen <= 32 * (usize)(__UINT32_MAX__));
#endif

	/* Compute HMAC state after processing P. */
	HMAC_SHA256_Init_internal(&Phctx, passwd, passwdlen,
	    tmp32, &tmp8[0], &tmp8[64]);

	/* Compute HMAC state after processing P and S. */
	memcpy(&PShctx, &Phctx, sizeof(HMAC_SHA256_CTX));
	HMAC_SHA256_Update_internal(&PShctx, salt, saltlen, tmp32);

	/* Iterate through the blocks. */
	for (i = 0; i * 32 < dkLen; i++) {
		/* Generate INT(i + 1). */
		be32enc(ivec, (u32)(i + 1));

		/* Compute U_1 = PRF(P, S || INT(i)). */
		memcpy(&hctx, &PShctx, sizeof(HMAC_SHA256_CTX));
		HMAC_SHA256_Update_internal(&hctx, ivec, 4, tmp32);
		HMAC_SHA256_Final_internal(U, &hctx, tmp32, tmp8);

		/* T_i = U_1 ... */
		memcpy(T, U, 32);

		for (j = 2; j <= c; j++) {
			/* Compute U_j. */
			memcpy(&hctx, &Phctx, sizeof(HMAC_SHA256_CTX));
			HMAC_SHA256_Update_internal(&hctx, U, 32, tmp32);
			HMAC_SHA256_Final_internal(U, &hctx, tmp32, tmp8);

			/* ... xor U_j ... */
			for (k = 0; k < 32; k++)
				T[k] ^= U[k];
		}

		/* Copy as many bytes as necessary into buf. */
		clen = dkLen - i * 32;
		if (clen > 32)
			clen = 32;
		memcpy(&buf[i * 32], T, clen);
	}

	/* Clean the stack. */
	clear_s_n(&Phctx, sizeof(HMAC_SHA256_CTX));
	clear_s_n(&PShctx, sizeof(HMAC_SHA256_CTX));
	clear_s_n(&hctx, sizeof(HMAC_SHA256_CTX));
	clear_s_n(tmp32, sizeof(u32) * 72);
	clear_s_n(tmp8, 96);
	clear_s_n(U, 32);
	clear_s_n(T, 32);
}

// stuff from lib/crypto etc.

static void (*smix_func)(u8 *, usize, u64, void *, void *) = 0;

static void
blkcpy(u32 * dest, const u32 * src, usize len)
{
	copy_b(src, (const byte *)src + len, dest, (const byte *)dest + len);
}

static void
blkxor(u32 * dest, const u32 * src, usize len)
{
	usize i;

	for (i = 0; i < len / 4; i++)
		dest[i] ^= src[i];
}

/**
 * salsa20_8(B):
 * Apply the salsa20/8 core to the provided block.
 */
static void
salsa20_8(u32 B[16])
{
	u32 x[16];
	usize i;

	blkcpy(x, B, 64);
	for (i = 0; i < 8; i += 2) {
#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x[ 4] ^= R(x[ 0]+x[12], 7);  x[ 8] ^= R(x[ 4]+x[ 0], 9);
		x[12] ^= R(x[ 8]+x[ 4],13);  x[ 0] ^= R(x[12]+x[ 8],18);

		x[ 9] ^= R(x[ 5]+x[ 1], 7);  x[13] ^= R(x[ 9]+x[ 5], 9);
		x[ 1] ^= R(x[13]+x[ 9],13);  x[ 5] ^= R(x[ 1]+x[13],18);

		x[14] ^= R(x[10]+x[ 6], 7);  x[ 2] ^= R(x[14]+x[10], 9);
		x[ 6] ^= R(x[ 2]+x[14],13);  x[10] ^= R(x[ 6]+x[ 2],18);

		x[ 3] ^= R(x[15]+x[11], 7);  x[ 7] ^= R(x[ 3]+x[15], 9);
		x[11] ^= R(x[ 7]+x[ 3],13);  x[15] ^= R(x[11]+x[ 7],18);

		/* Operate on rows. */
		x[ 1] ^= R(x[ 0]+x[ 3], 7);  x[ 2] ^= R(x[ 1]+x[ 0], 9);
		x[ 3] ^= R(x[ 2]+x[ 1],13);  x[ 0] ^= R(x[ 3]+x[ 2],18);

		x[ 6] ^= R(x[ 5]+x[ 4], 7);  x[ 7] ^= R(x[ 6]+x[ 5], 9);
		x[ 4] ^= R(x[ 7]+x[ 6],13);  x[ 5] ^= R(x[ 4]+x[ 7],18);

		x[11] ^= R(x[10]+x[ 9], 7);  x[ 8] ^= R(x[11]+x[10], 9);
		x[ 9] ^= R(x[ 8]+x[11],13);  x[10] ^= R(x[ 9]+x[ 8],18);

		x[12] ^= R(x[15]+x[14], 7);  x[13] ^= R(x[12]+x[15], 9);
		x[14] ^= R(x[13]+x[12],13);  x[15] ^= R(x[14]+x[13],18);
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
static void
blockmix_salsa8(const u32 * Bin, u32 * Bout, u32 * X, usize r)
{
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
static u64
integerify(const u32 * B, usize r)
{
	const u32 * X = B + (2 * r - 1) * 16;

	return (((u64)(X[1]) << 32) + X[0]);
}

static inline void
le32enc(void * pp, u32 x)
{
	u8 * p = (u8 *)pp;

	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (u8)((x >> 24) & 0xff);
}

static inline u32
le32dec(const void * pp)
{
	const u8 * p = (u8 const *)pp;

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
void
crypto_scrypt_smix(u8 * B, usize r, u64 N, void * _v, void * XY)
{
	u32 * X = XY;
	u32 * Y = (void *)((u8 *)(XY) + 128 * r);
	u32 * Z = (void *)((u8 *)(XY) + 256 * r);
	u32 * V = _v;
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
static int
crypto_scrypt_internal(const u8 * passwd, usize passwdlen,
    const u8 * salt, usize saltlen, u64 N, u32 _r, u32 _p,
    u8 * buf, usize buflen,
    void (*smix)(u8 *, usize, u64, void *, void *))
{
	void * B0, * V0, * XY0;
	u8 * B;
	u32 * V;
	u32 * XY;
	usize r = _r, p = _p;
	u32 i;

	/* Sanity-check parameters. */
	if ((r == 0) || (p == 0)) {
		goto err0;
	}
#if __SIZE_MAX__ > __UINT32_MAX__
	if (buflen > (((u64)(1) << 32) - 1) * 32) {
		goto err0;
	}
#endif
	if ((u64)(r) * (u64)(p) >= (1 << 30)) {
		goto err0;
	}
	if (((N & (N - 1)) != 0) || (N < 2)) {
		goto err0;
	}
	if ((r > __SIZE_MAX__ / 128 / p) ||
#if __SIZE_MAX__ / 256 <= __UINT32_MAX__
	    (r > (__SIZE_MAX__ - 64) / 256) ||
#endif
	    (N > __SIZE_MAX__ / 128 / r)) {
		goto err0;
	}

	/* Allocate memory. */
#ifdef HAVE_POSIX_MEMALIGN
	if ((errno = posix_memalign(&B0, 64, 128 * r * p)) != 0)
		goto err0;
	B = (u8 *)(B0);
	if ((errno = posix_memalign(&XY0, 64, 256 * r + 64)) != 0)
		goto err1;
	XY = (u32 *)(XY0);
#if !defined(MAP_ANON) || !defined(HAVE_MMAP)
	if ((errno = posix_memalign(&V0, 64, (usize)(128 * r * N))) != 0)
		goto err2;
	V = (u32 *)(V0);
#endif
#else
	if ((B0 = os_allocate((size)(128 * r * p + 63))) == 0) // TODO conversion
		goto err0;
	B = (u8 *)(((uptr)(B0) + 63) & ~ (uptr)(63));
	if ((XY0 = os_allocate((size)(256 * r + 64 + 63))) == 0) // TODO conversion
		goto err1;
	XY = (u32 *)(((uptr)(XY0) + 63) & ~ (uptr)(63));
#if !defined(MAP_ANON) || !defined(HAVE_MMAP)
	if ((V0 = os_allocate((size)(128 * r * N + 63))) == 0) // TODO conversion
		goto err2;
	V = (u32 *)(((uptr)(V0) + 63) & ~ (uptr)(63));
#endif
#endif
#if defined(MAP_ANON) && defined(HAVE_MMAP)
	if ((V0 = mmap(NULL, (usize)(128 * r * N), PROT_READ | PROT_WRITE,
#ifdef MAP_NOCORE
	    MAP_ANON | MAP_PRIVATE | MAP_NOCORE,
#else
	    MAP_ANON | MAP_PRIVATE,
#endif
	    -1, 0)) == MAP_FAILED)
		goto err2;
	V = (u32 *)(V0);
#endif

	/* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
#if 1
	PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, 1, B, p * 128 * r);
#else
	buf8 tbuf = (buf8){B, B, B + p * 128 * r};
	pbkdf2_sha256((utf8){passwd, passwd + passwdlen}, (view8){salt, salt + saltlen}, 1, &tbuf);
#endif

	/* 2: for i = 0 to p - 1 do */
	for (i = 0; i < p; i++) {
		/* 3: B_i <-- MF(B_i, N) */
		smix(&B[i * 128 * r], r, N, V, XY);
	}

	/* 5: DK <-- PBKDF2(P, B, 1, dkLen) */
#if 1
	PBKDF2_SHA256(passwd, passwdlen, B, p * 128 * r, 1, buf, buflen);
#else
	tbuf = (buf8){buf, buf, buf + buflen};
	pbkdf2_sha256((utf8){passwd, passwd + passwdlen}, (view8){B, B + p * 128 * r}, 1, &tbuf);
#endif

	/* Free memory. */
#if defined(MAP_ANON) && defined(HAVE_MMAP)
	if (munmap(V0, (usize)(128 * r * N)))
		goto err2;
#else
	// free(V0); TODO
#endif
	// free(XY0); TODO
	// free(B0); TODO

	/* Success! */
	return (0);

err2:
	// free(XY0); TODO
err1:
	// free(B0); TODO
err0:
	/* Failure! */
	return (-1);
}

#define TESTLEN 64
static struct scrypt_test {
	const char * passwd;
	const char * salt;
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

static int testsmix(void (*smix)(u8 *, usize, u64, void *, void *))
{
  u8 hbuf[TESTLEN];

  // Perform the computation.
  if (crypto_scrypt_internal(
      (const u8 *)testcase.passwd, (usize)ascii_length_of(testcase.passwd), // TODO conversion
      (const u8 *)testcase.salt, (usize)ascii_length_of(testcase.salt), // TODO conversion
      testcase.N, testcase.r, testcase.p, hbuf, TESTLEN, smix))
    return (-1);

  // Does it match?
  return equal_b_n(testcase.result, hbuf, TESTLEN) ? 0 : 1;
}

/**
 * crypto_scrypt(passwd, passwdlen, salt, saltlen, N, r, p, buf, buflen):
 * Compute scrypt(passwd[0 ... passwdlen - 1], salt[0 ... saltlen - 1], N, r,
 * p, buflen) and write the result into buf.  The parameters r, p, and buflen
 * must satisfy 0 < r * p < 2^30 and buflen <= (2^32 - 1) * 32.  The parameter
 * N must be a power of 2 greater than 1.
 *
 * Return 0 on success; or -1 on error.
 */
int crypto_scrypt(const u8 *passwd, size passwdlen,
                  const u8 *salt, size saltlen,
                  u64 N, u32 _r, u32 _p,
                  u8 *buf, size buflen)
{
  // Ensure generic smix works.
  if (!testsmix(crypto_scrypt_smix)) {
    smix_func = crypto_scrypt_smix;
    return crypto_scrypt_internal(passwd, (usize)passwdlen,
                                  salt, (usize)saltlen, N, _r, _p,
                                  buf, (usize)buflen, smix_func);
  }

  print(utf8("fatal: cannot derive passwords - scrypt smix failed test"));
  exit_with_failure();
}
