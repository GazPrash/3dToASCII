#include "renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <sys/ioctl.h>
#include <unistd.h>

#include "extractor.h"

namespace {

volatile std::sig_atomic_t keepRunning = 1;

void handleStopSignal(int) {
  keepRunning = 0;
}

Vec3 rotateXz(const Vec3 &pt, double angle) {
  const double c = std::cos(angle);
  const double s = std::sin(angle);

  return {
      pt.x * c - pt.z * s,
      pt.y,
      pt.z * c + pt.x * s,
  };
}

Vec3 translateZ(const Vec3 &pt, double dz) {
  return {pt.x, pt.y, pt.z + dz};
}

std::pair<int, int> terminalSize() {
  winsize size{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 &&
      size.ws_row > 0) {
    return {static_cast<int>(size.ws_col), static_cast<int>(size.ws_row)};
  }

  return {100, 40};
}

char depthChar(double z) {
  static const std::string chars = ".,:;ox%#@";
  const double t = std::clamp((4.0 - z) / 2.5, 0.0, 1.0);
  const size_t index = static_cast<size_t>(t * (chars.size() - 1));
  return chars[index];
}

void renderFrame(const Model &model, double angle, int width, int height) {
  std::vector<std::string> frame(height, std::string(width, ' '));
  std::vector<double> zBuffer(width * height,
                              std::numeric_limits<double>::infinity());

  for (const Vec3 &vertex : model.vertices) {
    const Vec3 rotated = rotateXz(vertex, angle);
    const Vec3 translated = translateZ(rotated, Z_OFFSET);
    if (translated.z <= 0.001) continue;

    const double projectedX = translated.x / translated.z;
    const double projectedY = translated.y / translated.z;
    const int x = static_cast<int>(((projectedX + 1.0) / 2.0) * (width - 1));
    const int y =
        static_cast<int>((1.0 - ((projectedY + 1.0) / 2.0)) * (height - 1));

    if (x < 0 || x >= width || y < 0 || y >= height) continue;

    const int zIndex = y * width + x;
    if (translated.z < zBuffer[zIndex]) {
      zBuffer[zIndex] = translated.z;
      frame[y][x] = depthChar(translated.z);
    }
  }

  std::ostringstream output;
  output << "\x1b[H";
  for (const std::string &row : frame) {
    output << row << '\n';
  }
  std::cout << output.str() << std::flush;
}

}  // namespace

int runRenderer(const std::string &objPath, int maxFrames) {
  std::signal(SIGINT, handleStopSignal);
  std::signal(SIGTERM, handleStopSignal);

  Model model;
  if (!readObjFile(objPath, model)) {
    return 1;
  }

  std::cerr << "Loaded " << model.vertices.size() << " vertices and "
            << model.faces.size() << " faces from " << objPath << "\n";

  const auto [terminalWidth, terminalHeight] = terminalSize();
  const int width = std::max(20, terminalWidth);
  const int height = std::max(10, terminalHeight - 1);
  const auto frameTime = std::chrono::milliseconds(
      static_cast<int>(std::round(1000.0 / FPS)));

  std::cout << "\x1b[2J\x1b[?25l";
  double angle = 0.0;
  int frame = 0;

  while (keepRunning && (maxFrames == 0 || frame < maxFrames)) {
    renderFrame(model, angle, width, height);
    angle += SPIN_SPEED / FPS;
    frame++;
    std::this_thread::sleep_for(frameTime);
  }

  std::cout << "\x1b[?25h" << std::flush;
  return 0;
}
