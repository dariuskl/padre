// This is free and unencumbered software released into the public domain.

// Determines the system-specific implementations and startup-files to be used.
// Include this file from the file containing your executable's entry point.

#if __linux__ && __amd64__
  #include "linux_amd64.c"
#else
  #error "unsupported"
#endif

static size print_int(long long num, int base) {
  static u8 chars[] = "0123456789abcdef";

  if (base < 2 || base > 16)
    return -1;

  u8 buf_[16];
  buf8 buf = buf8(buf_);
  buf.eod = buf.begin + 16;

  num = num < 0 ? -num : num;

  for (; num > 0; num /= base) {
    --buf.eod;
    *buf.eod = chars[num % base];
  }

  if (num < 0) {
    --buf.eod;
    *buf.eod = '-';
    num = -num;
  }

  return print(((utf8){buf.eod, buf.end}));
}

size print_i32(i32 num) {
  return print_int(num, 10);
}

size print_ptr(const void *ptr) {
  return print_int((long long)(uptr)ptr, 16);
}

bool scan_i32(utf8 *buf, i32 *num) {
  int tmp = 0;
  for (u32 c = utf8_nextch(buf); c; c = utf8_nextch(buf)) {
    if (c >= u8'0' && c <= u8'9') {
      tmp = tmp * 10 + (i32)c - '0';
    }
  }
  *num = tmp;
  return true;
}
