#pragma once

#include "Mesh.hpp"

#include <iosfwd>
#include <string>

// Minimal Wavefront OBJ support: v, vt, vn, f (any polygon, fan-triangulated,
// negative indices allowed). Missing normals are generated (smooth).
bool parseObj(std::istream& input, Mesh& out, std::string* error = nullptr);
bool loadObj(const std::string& path, Mesh& out, std::string* error = nullptr);
