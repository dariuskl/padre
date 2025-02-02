// This is free and unencumbered software released into the public domain.

// A program computing sha256 sums the same way sha256sum does.
// Used to test the sha256 implementation used in padre.

#define NOSTDLIB_IMPLEMENTATION
#include "nostdlib.c"
#include "sha256.c"

u8 input_buffer[512];

buf8 read_stdin(void) {
  buf8 buf = buf8(input_buffer);
  while (scan(&buf) > 0) {
  }
  return buf;
}

i32 entry(i32 /*argc*/, u8 */*argv*/[], u8 */*envp*/[]) {
  buf8 in = read_stdin();
  sha256_hash digest = sha256_calculate(buf_to_view(in));

  const u8 hex[16] = "0123456789abcdef";
  u8 out[68];
  for (int i = 0; i < 32; i++) {
    out[(i * 2) + 1] = hex[digest.bytes[i] & 0xf];
    out[i * 2] = hex[(digest.bytes[i] >> 4) & 0xf];
  }
  out[64] = ' ';
  out[65] = ' ';
  out[66] = '-';
  out[67] = '\n';
  print(((utf8){out, out + 68}));

  return 0;
}
