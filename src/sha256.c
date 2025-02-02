// Implementation of SHA256 hash function.
// Original author: Tom St Denis, tomstdenis@gmail.com, http://libtom.org
// Modified by WaterJuice retaining Public Domain license.
//  https://github.com/WaterJuice/WjCryptLib/blob/Version_2.3.0/lib/WjCryptLib_Sha256.c
// Modified by Darius Kellermann, kellermann@pm.me, retaining Public Domain license.
//
//  This is free and unencumbered software released into the public domain - June 2013 waterjuice.org

static inline void put_unaligned_be_u32(byte vec[static 4], u32 val) {
  vec[0] = (byte)(val >> 24);
  vec[1] = (byte)(val >> 16);
  vec[2] = (byte)(val >>  8);
  vec[3] = (byte) val       ;
}

static inline u32 get_unaligned_be_u32(const byte vec[static 4]) {
  return vec[0] << 24 | vec[1] << 16 | vec[2] << 8 | vec[3];
}

static inline void put_unaligned_be_u64(byte vec[static 8], u64 val) {
  vec[0] = (byte)(val >> 56);
  vec[1] = (byte)(val >> 48);
  vec[2] = (byte)(val >> 40);
  vec[3] = (byte)(val >> 32);
  vec[4] = (byte)(val >> 24);
  vec[5] = (byte)(val >> 16);
  vec[6] = (byte)(val >>  8);
  vec[7] = (byte) val       ;
}

// The K array, SHA256 round constants
static const u32 K[64] = {
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

#define SHA256_BLOCK_SIZE          64

// various logical functions
#define ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
#define Ch(x, y, z)       ((x & (y ^ z)) ^ z)
#define Maj(x, y, z)      (((x | y) & z) | (x & y))
#define S(x, n)           ror((x), (n))
#define R(x, n)           (((x) & 0xffffffff) >> (n))
#define Sigma0(x)         (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)         (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)         (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)         (S(x, 17) ^ S(x, 19) ^ R(x, 10))

typedef struct {
  size length;  // the number of bits processed so far
  size curlen;
  u32 state[8];
  u8 buf[64];
} sha256_context;

// Transform function, compress 512-bits
void sha256_transform(sha256_context* ctx, const u8* buffer) {
  u32 W[64] = {};
  // copy the state into 512-bits into W[0..15]
  for (int i = 0; i < 16; ++i) {
    W[i] = get_unaligned_be_u32(buffer + (4 * i));
  }

  // fill W[16..63]
  for (int i = 16; i < 64; ++i) {
    W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
  }

  u32 S[8] = {};
  // copy state into S
  for (int i = 0; i < 8; ++i) {
    S[i] = ctx->state[i];
  }

  // compress
  for (int i = 0; i < 64; ++i) {
    u32 t0 = S[7] + Sigma1(S[4]) + Ch(S[4], S[5], S[6]) + K[i] + W[i];
    u32 t1 = Sigma0(S[0]) + Maj(S[0], S[1], S[2]);
    S[3] += t0; S[7] = t0 + t1;

    u32 t = S[7];
    S[7] = S[6];
    S[6] = S[5];
    S[5] = S[4];
    S[4] = S[3];
    S[3] = S[2];
    S[2] = S[1];
    S[1] = S[0];
    S[0] = t;
  }

  // feedback
  for (int i = 0; i < 8; ++i) {
    ctx->state[i] = ctx->state[i] + S[i];
  }
}

// Initialises a SHA256 context. Use this to initialise/reset a context.
sha256_context sha256_init(void) {
  sha256_context ctx = {};
  ctx.length = 0;
  ctx.state[0] = 0x6a09e667;
  ctx.state[1] = 0xbb67ae85;
  ctx.state[2] = 0x3c6ef372;
  ctx.state[3] = 0xa54ff53a;
  ctx.state[4] = 0x510e527f;
  ctx.state[5] = 0x9b05688c;
  ctx.state[6] = 0x1f83d9ab;
  ctx.state[7] = 0x5be0cd19;
  ctx.curlen = 0;
  return ctx;
}

// Adds data to the SHA256 context. This will process the data and update the
// internal state of the context. Keep on calling this function until all the
// data has been added. Then call sha256_finalize to calculate the hash.
void sha256_update(sha256_context* ctx, const u8* buffer, size buffer_size) {
  if (ctx->curlen > size_of(ctx->buf)) {
    return;
  }

  while (buffer_size > 0) {
    if (ctx->curlen == 0 && buffer_size >= SHA256_BLOCK_SIZE) {
      sha256_transform(ctx, (u8 *)buffer);
      ctx->length += SHA256_BLOCK_SIZE * 8;
      buffer = (u8 *)buffer + SHA256_BLOCK_SIZE;
      buffer_size -= SHA256_BLOCK_SIZE;
    } else {
      size n = min(buffer_size, (SHA256_BLOCK_SIZE - ctx->curlen));
      copy_b(buffer, buffer + n, ctx->buf + ctx->curlen, ctx->buf + ctx->curlen + n);
      ctx->curlen += n;
      buffer = (u8 *)buffer + n;
      buffer_size -= n;
      if (ctx->curlen == SHA256_BLOCK_SIZE) {
        sha256_transform(ctx, ctx->buf);
        ctx->length += 8 * SHA256_BLOCK_SIZE;
        ctx->curlen = 0;
      }
    }
  }
}

#define SHA256_HASH_SIZE           (256 / 8)

typedef struct {
  u8 bytes[SHA256_HASH_SIZE];
} sha256_hash;

// Performs the final calculation of the hash and returns the digest (32 byte
// buffer containing 256bit hash). After calling this, Sha256Initialised must
// be used to reuse the context.
void sha256_finalize(sha256_context* ctx, sha256_hash* digest) {
  if (ctx->curlen >= size_of(ctx->buf)) {
    return;
  }

  // increase the length of the message
  ctx->length += ctx->curlen * 8;

  // append the '1' bit
  ctx->buf[ctx->curlen++] = 0x80;

  // If the length is currently above 56 bytes we append zeros then compress.
  // Then we can fall back to padding zeros and length encoding like normal.
  if (ctx->curlen > 56) {
    while (ctx->curlen < 64) {
      ctx->buf[ctx->curlen++] = 0;
    }
    sha256_transform(ctx, ctx->buf);
    ctx->curlen = 0;
  }

  // pad up to 56 bytes of zeroes
  while (ctx->curlen < 56) {
    ctx->buf[ctx->curlen++] = 0;
  }

  // store length
  put_unaligned_be_u64(ctx->buf + 56, (u64)ctx->length);
  sha256_transform(ctx, ctx->buf);

  // copy output
  for (int i = 0; i < 8; ++i) {
    put_unaligned_be_u32(digest->bytes + (4 * i), ctx->state[i]);
  }
}

// Calculates the SHA256 hash of the data in `v`.
sha256_hash sha256_calculate(view8 v) {
  sha256_hash digest = {};
  sha256_context ctx = sha256_init();
  sha256_update(&ctx, v.begin, view8_len(v));
  sha256_finalize(&ctx, &digest);
  return digest;
}

#undef Ch
#undef Maj
#undef R

typedef struct {
  sha256_context ictx;
  sha256_context octx;
} hmac_sha256_context;

hmac_sha256_context hmac_sha256_init(view8 key) {
  // if the key is larger than a sha256 block, it must be hashed
  sha256_hash key_hash;
  if (view8_len(key) > SHA256_BLOCK_SIZE) {
    key_hash = sha256_calculate(key);
    key.begin = key_hash.bytes;
    key.end = key_hash.bytes + size_of(key_hash.bytes);
  }

  hmac_sha256_context ctx;
  ctx.ictx = sha256_init();
  ctx.octx = sha256_init();

  u8 tmp[SHA256_BLOCK_SIZE];
  // key bytes XOR 0x36 bytes
  fill(tmp, tmp + size_of(tmp), 0x36);
  for (int i = 0; i < view8_len(key); ++i) {
    tmp[i] ^= key.begin[i];
  }
  sha256_update(&ctx.ictx, tmp, 64);

  // key bytes XOR 0x5c bytes
  fill(tmp, tmp + size_of(tmp), 0x5c);
  for (int i = 0; i < view8_len(key); ++i) {
    tmp[i] ^= key.begin[i];
  }
  sha256_update(&ctx.octx, tmp, 64);

  return ctx;
}

void hmac_sha256_update(hmac_sha256_context *ctx,
                        const u8 *buffer, size buffer_len) {
  sha256_update(&ctx->ictx, buffer, buffer_len);
}

sha256_hash hmac_sha256_finalize(hmac_sha256_context *ctx) {
  // finalize inner hash
  sha256_hash inner_hash;
  sha256_finalize(&ctx->ictx, &inner_hash);

  // feed inner hash to outer hash
  sha256_update(&ctx->octx, inner_hash.bytes, SHA256_HASH_SIZE);

  // finalize outer hash
  sha256_hash outer_hash;
  sha256_finalize(&ctx->octx, &outer_hash);

  return outer_hash;
}

void pbkdf2_sha256(utf8 password, view8 salt, size cost, buf8 *out) {
  // initialize context with the password
  hmac_sha256_context ctx = hmac_sha256_init(utf8_to_view(password));

  // create a second context after salting
  hmac_sha256_context salted_ctx = ctx;
  hmac_sha256_update(&salted_ctx, salt.begin, view8_len(salt));

  u8 block[32];

  // iterate block-wise
  for (int i = 0; i * 32 < buf8_capacity(*out); ++i) {
    // encode block number as 32-bit big-endian into byte array
    union { u32 uint; u8 bytes[4]; } block_no;
    block_no.uint = __builtin_bswap32((u32)i + 1);

    // update salted context with block number
    hmac_sha256_context block_ctx = salted_ctx;
    hmac_sha256_update(&block_ctx, block_no.bytes, size_of(block_no));
    // obtain initial block hash
    sha256_hash block_hash = hmac_sha256_finalize(&block_ctx);

    // init final block with initial hash
    copy_b(block_hash.bytes, block_hash.bytes + size_of(block_hash.bytes),
           block, block + size_of(block));

    for (int j = 2; j <= cost; ++j) {
      // update unsalted context with block data
      block_ctx = ctx;
      hmac_sha256_update(&block_ctx, block_hash.bytes, size_of(block_hash.bytes));
      // update block hash
      block_hash = hmac_sha256_finalize(&block_ctx);

      // update final block by XORing with block_hash
      for (int k = 0; k < 32; ++k) {
        block[k] ^= block_hash.bytes[k];
      }
    }

    size remaining_bytes = buf8_capacity(*out) - i * 32;
    size bytes_to_copy = min(remaining_bytes, size_of(block));
    out->eod = copy_b(block, block + bytes_to_copy, out->eod, out->end);
  }
}

// TODO clean up stack memory after use
