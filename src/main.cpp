#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include "renderer.h"

namespace {

bool parseInteger(const std::string &text, int &result) {
  if (text.empty()) return false;

  char *end = nullptr;
  errno = 0;
  const long value = std::strtol(text.c_str(), &end, 10);

  if (end == text.c_str() || *end != '\0' || errno == ERANGE ||
      value < std::numeric_limits<int>::min() ||
      value > std::numeric_limits<int>::max()) {
    return false;
  }

  result = static_cast<int>(value);
  return true;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " <obj-file> [--frames non-negative-count]\n";
    return 1;
  }

  std::string objPath = argv[1];
  int maxFrames = 0;

  for (int i = 2; i < argc; i++) {
    const std::string arg = argv[i];
    if (arg == "--frames") {
      if (i + 1 >= argc || !parseInteger(argv[i + 1], maxFrames) ||
          maxFrames < 0) {
        std::cerr << "Usage: " << argv[0]
                  << " [obj-file] [--frames non-negative-count]\n";
        return 1;
      }
      i++;
    } else {
      objPath = arg;
    }
  }

  return runRenderer(objPath, maxFrames);
}
