// Test runner. Build: cmake --build build --target tests    Run: ./build/tests

#include <cstdio>

#include "test_common.hpp"

void run_decode_tests();
void run_cpu_tests();

int main() {
  run_decode_tests();
  run_cpu_tests();

  std::printf("%d checks, %d failed\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
