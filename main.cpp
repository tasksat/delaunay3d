#include <print>
#include <vector>

#include "delaunay/delaunay.hpp"
#include "utils/log.hpp"

namespace tsklog = tasksat::log;

int main() {
    std::vector<Vec3> points = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
    };

    const Tetrahedra mesh = tetrahedralize(points);

    tsklog::info("vertices: {}", std::ssize(mesh.vertices));
}