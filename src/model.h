#pragma once

#include <vector>

struct Vec3 {
  double x;
  double y;
  double z;
};

struct Model {
  std::vector<Vec3> vertices;
  std::vector<std::vector<int>> faces;
};

constexpr double PI = 3.14159265358979323846;
constexpr double FPS = 60.0;
constexpr double SPIN_SPEED = PI * 0.5;
constexpr double MODEL_FIT_SIZE = 2.0;
constexpr double Z_OFFSET = 3.0;
