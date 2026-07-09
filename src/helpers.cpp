#include <chrono>
#include <rad-tests-app/helpers.hpp>

int64_t getUnixMs(void) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
