// Shared test plumbing — no framework, just counters and two macros.
// Plain assert() is not used because it vanishes under NDEBUG and aborts on
// the first failure; these report every failure with file:line and keep going.

#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include <cstdio>

inline int checks = 0;
inline int failures = 0;

// generic condition check
#define CHECK(cond)                                                          \
  do {                                                                       \
    checks++;                                                                \
    if (!(cond)) {                                                           \
      failures++;                                                            \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);            \
    }                                                                        \
  } while (0)

// numeric comparison, prints got/want on failure (values compared as i64 so
// u32 fields and negative i32 immediates both keep their meaning)
#define CHECK_EQ(got, want)                                                  \
  do {                                                                       \
    checks++;                                                                \
    long long g = (long long)(got);                                          \
    long long w = (long long)(want);                                         \
    if (g != w) {                                                            \
      failures++;                                                            \
      std::printf("FAIL %s:%d  %s == %s  (got 0x%llX (%lld), want 0x%llX "   \
                  "(%lld))\n",                                               \
                  __FILE__, __LINE__, #got, #want, (unsigned long long)g, g, \
                  (unsigned long long)w, w);                                 \
    }                                                                        \
  } while (0)

#endif // TEST_COMMON_HPP
