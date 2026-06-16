#pragma once

#include "utils/eigen.hpp"

#include <array>
#include <span>
#include <string>
#include <vector>

using Index = std::uint32_t;

struct Tetrahedra {
    std::vector<Vec3> vertices;
    std::vector<Index> indices;
};

Tetrahedra tetrahedralize(std::span<const Vec3> points);
