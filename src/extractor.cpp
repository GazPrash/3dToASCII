#include "extractor.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {

std::string trim(const std::string &text) {
  const size_t start = text.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";

  const size_t end = text.find_last_not_of(" \t\r\n");
  return text.substr(start, end - start + 1);
}

std::vector<std::string> splitWhitespace(const std::string &line) {
  std::istringstream stream(line);
  std::vector<std::string> parts;
  std::string part;

  while (stream >> part) {
    parts.push_back(part);
  }

  return parts;
}

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

bool parseDouble(const std::string &text, double &result) {
  if (text.empty()) return false;

  char *end = nullptr;
  errno = 0;
  const double value = std::strtod(text.c_str(), &end);

  if (end == text.c_str() || *end != '\0' || errno == ERANGE ||
      !std::isfinite(value)) {
    return false;
  }

  result = value;
  return true;
}

int parseFaceIndex(const std::string &value, size_t vertexCount) {
  const size_t slash = value.find('/');
  const std::string raw = value.substr(0, slash);
  int index = 0;
  if (!parseInteger(raw, index)) {
    return -1;
  }

  if (index == 0) return -1;
  return index > 0 ? index - 1 : static_cast<int>(vertexCount) + index;
}

Model parseObj(const std::string &objText) {
  Model model;
  std::istringstream stream(objText);
  std::string line;

  while (std::getline(stream, line)) {
    const std::string cleanLine = trim(line);
    if (cleanLine.empty() || cleanLine[0] == '#') continue;

    const std::vector<std::string> parts = splitWhitespace(cleanLine);
    if (parts.empty()) continue;

    if (parts[0] == "v" && parts.size() >= 4) {
      double x = 0.0;
      double y = 0.0;
      double z = 0.0;
      if (parseDouble(parts[1], x) && parseDouble(parts[2], y) &&
          parseDouble(parts[3], z)) {
        model.vertices.push_back({x, y, z});
      }
    } else if (parts[0] == "f" && parts.size() >= 3) {
      std::vector<int> face;
      for (size_t i = 1; i < parts.size(); i++) {
        const int index = parseFaceIndex(parts[i], model.vertices.size());
        if (index >= 0 && index < static_cast<int>(model.vertices.size())) {
          face.push_back(index);
        }
      }

      if (face.size() >= 2) {
        model.faces.push_back(face);
      }
    }
  }

  return model;
}

bool readTextFile(const std::string &filePath, std::string &contents) {
  std::ifstream file(filePath);
  if (!file) {
    std::cerr << "Could not open OBJ file: " << filePath << "\n";
    return false;
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();

  if (file.bad()) {
    std::cerr << "Could not finish reading OBJ file: " << filePath << "\n";
    return false;
  }

  contents = buffer.str();
  return true;
}

std::vector<Vec3> normalizeVertices(const std::vector<Vec3> &vertices,
                                    double fitSize = MODEL_FIT_SIZE) {
  if (vertices.empty()) return {};

  Vec3 minPt{
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
  };
  Vec3 maxPt{
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
  };

  for (const Vec3 &pt : vertices) {
    minPt.x = std::min(minPt.x, pt.x);
    minPt.y = std::min(minPt.y, pt.y);
    minPt.z = std::min(minPt.z, pt.z);
    maxPt.x = std::max(maxPt.x, pt.x);
    maxPt.y = std::max(maxPt.y, pt.y);
    maxPt.z = std::max(maxPt.z, pt.z);
  }

  const Vec3 center{
      (minPt.x + maxPt.x) / 2.0,
      (minPt.y + maxPt.y) / 2.0,
      (minPt.z + maxPt.z) / 2.0,
  };
  const double maxDimension =
      std::max({maxPt.x - minPt.x, maxPt.y - minPt.y, maxPt.z - minPt.z});
  const double scale = maxDimension > 0.0 ? fitSize / maxDimension : 1.0;

  std::vector<Vec3> normalized;
  normalized.reserve(vertices.size());
  for (const Vec3 &pt : vertices) {
    normalized.push_back({
        (pt.x - center.x) * scale,
        (pt.y - center.y) * scale,
        (pt.z - center.z) * scale,
    });
  }

  return normalized;
}

}  // namespace

bool readObjFile(const std::string &filePath, Model &model) {
  std::string objText;
  if (!readTextFile(filePath, objText)) {
    return false;
  }

  model = parseObj(objText);
  if (model.vertices.empty()) {
    std::cerr << "OBJ file did not contain any vertices: " << filePath
              << "\n";
    return false;
  }

  model.vertices = normalizeVertices(model.vertices);
  return true;
}
