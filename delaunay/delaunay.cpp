#include "delaunay/delaunay.hpp"
#include "delaunay/builder.hpp"

#include <algorithm>
#include <cmath>

Tetrahedra tetrahedralize(std::span<const Vec3> points) {
    return DelaunayBuilder(points).run();
}