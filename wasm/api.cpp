#include <cstdint>
#include <vector>

#include "delaunay/delaunay.hpp"

namespace {

std::vector<double> g_vertices;
std::vector<std::uint32_t> g_indices;

} // namespace

extern "C" {

int dlny_build(const double* flat_points, int point_count) {
    if (flat_points == nullptr || point_count <= 0) {
        return 0;
    }
    std::vector<Vec3> points;
    points.reserve(point_count);
    for (int i = 0; i < point_count; i++) {
        points.emplace_back(
            flat_points[3 * i + 0],
            flat_points[3 * i + 1],
            flat_points[3 * i + 2]
        );
    }

    Tetrahedra mesh = tetrahedralize(points);

    g_vertices.clear();
    g_vertices.reserve(mesh.vertices.size());
    for (const Vec3& p : mesh.vertices) {
        g_vertices.push_back(p.x());
        g_vertices.push_back(p.y());
        g_vertices.push_back(p.z());
    }
    g_indices = mesh.indices;
    return 1;
}

int dlny_vertex_count() {
    return static_cast<int>(g_vertices.size() / 3);
}

int dlny_tet_count() {
    return static_cast<int>(g_indices.size() / 4);
}

const double* dlny_get_vertices() {
    return g_vertices.data();
}

const std::uint32_t* dlny_get_indices() {
    return g_indices.data();
}
}