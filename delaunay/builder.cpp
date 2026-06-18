#include "delaunay/builder.hpp"
#include "utils/log.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace {

namespace tlog = tasksat::log;

constexpr double EPS = 1e-12;

double signed_volume(
    const Eigen::Ref<const Vec3>& v0,
    const Eigen::Ref<const Vec3>& v1,
    const Eigen::Ref<const Vec3>& v2,
    const Eigen::Ref<const Vec3>& v3
) {
    Mat3 m;
    m.col(0) = v1 - v0;
    m.col(1) = v2 - v0;
    m.col(2) = v3 - v0;
    return m.determinant() / 6.0;
}

// x^2 + y^2 + z^2 + ax + by + cz + d = 0
int in_sphere(const Vec3& p, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    Mat4 A;
    auto set_row = [&](int row, const Vec3& q) -> void {
        const Vec3 v = q - p;
        A(row, 0) = v.x();
        A(row, 1) = v.y();
        A(row, 2) = v.z();
        A(row, 3) = -v.squaredNorm();
    };
    set_row(0, p0);
    set_row(1, p1);
    set_row(2, p2);
    set_row(3, p3);

    double detA = A.determinant();

    if (std::abs(detA) <= EPS) {
        return 0;
    }

    return detA > 0.0 ? 1 : -1;
}

Face make_face(Index i0, Index i1, Index i2) {
    std::array<Index, 3> f0{i0, i1, i2};
    std::array<Index, 3> f1{i1, i2, i0};
    std::array<Index, 3> f2{i2, i0, i1};
    return Face{std::min({f0, f1, f2})};
}

std::array<std::array<Index, 4>, 4> tet_faces(std::span<const Index, 4> idx) {
    const Index i0 = idx[0];
    const Index i1 = idx[1];
    const Index i2 = idx[2];
    const Index i3 = idx[3];

    return {{
        {i0, i1, i2, i3},
        {i0, i2, i3, i1},
        {i0, i3, i1, i2},
        {i1, i3, i2, i0},
    }};
}

constexpr Index GHOST = std::numeric_limits<Index>::max();

bool is_ghost(std::span<const Index, 4> idx) {
    return std::any_of(std::begin(idx), std::end(idx), [](Index i) { return i == GHOST; });
}

} // namespace

DelaunayBuilder::DelaunayBuilder(std::span<const Vec3> points) {
    mesh.vertices.assign(std::begin(points), std::end(points));
}

Tetrahedra DelaunayBuilder::run() {
    if (mesh.vertices.size() < 4) {
        return mesh;
    }

    const auto init = initialize();
    if (not init) {
        return export_mesh();
    }

    const Index N = static_cast<Index>(mesh.vertices.size());
    for (Index i = 0; i < N; i++) {
        if (std::any_of(std::begin(*init), std::end(*init), [&](Index j) { return i == j; })) {
            continue;
        }
        const auto tid = find_conflict(i);
        if (not tid) {
            tlog::warning("run(): no conflicting tet found. (i: {})", i);
            continue;
        }

        insert_vertex(i, *tid);
    }

    return export_mesh();
}

void DelaunayBuilder::add_point(const Vec3& p) {
    mesh.vertices.push_back(p);
}

std::optional<std::array<Index, 4>> DelaunayBuilder::initialize() {
    const Index N = static_cast<Index>(mesh.vertices.size());

    std::optional<std::array<Index, 4>> init;

    [&] {
        for (Index i = 0; i < N; i++) {
            for (Index j = i + 1; j < N; j++) {
                for (Index k = j + 1; k < N; k++) {
                    for (Index l = k + 1; l < N; l++) {
                        const double volume = signed_volume(
                            mesh.vertices[i],
                            mesh.vertices[j],
                            mesh.vertices[k],
                            mesh.vertices[l]
                        );

                        if (std::abs(volume) <= EPS) {
                            continue;
                        }

                        std::array<Index, 4> tet{i, j, k, l};
                        if (volume < 0.0) {
                            std::swap(tet[0], tet[1]);
                        }

                        init = tet;
                        return;
                    }
                }
            }
        }
    }();

    if (not init) {
        tlog::warning("DelaunayBuilder::initialize() failed: no tetrahedron found from initial input.");
        return std::nullopt;
    }

    const auto [i0, i1, i2, i3] = *init;

    add_tet(i0, i1, i2, i3);

    auto add_ghost = [&](Index u, Index v, Index w) -> void { add_tet(w, v, u, GHOST); };

    add_ghost(i0, i1, i2);
    add_ghost(i0, i2, i3);
    add_ghost(i0, i3, i1);
    add_ghost(i1, i3, i2);

    return init;
}

Tetrahedra DelaunayBuilder::export_mesh() const {
    Tetrahedra out;
    out.vertices = mesh.vertices;
    out.indices.reserve(tets.size() * 4);
    for (const auto& [idx, alive] : tets) {
        if (not alive || is_ghost(idx)) {
            continue;
        }
        out.indices.push_back(idx[0]);
        out.indices.push_back(idx[1]);
        out.indices.push_back(idx[2]);
        out.indices.push_back(idx[3]);
    }

    return out;
}

std::optional<std::size_t> DelaunayBuilder::find_conflict(Index i) const {
    for (std::size_t tid = 0; tid < tets.size(); tid++) {
        if (conflicts(i, tid)) {
            return tid;
        }
    }
    return std::nullopt;
}

bool DelaunayBuilder::conflicts(Index i, std::size_t tid) const {
    if (tid >= tets.size()) {
        tlog::warning("conflicts(): invalide tid. (tid: {})", tid);
        return false;
    }
    const auto& [idx, alive] = tets[tid];

    if (not alive) {
        return false;
    }

    if (not is_ghost(idx)) {
        const auto [i0, i1, i2, i3] = idx;
        return in_sphere(i, i0, i1, i2, i3) > 0;
    }

    for (const auto [i0, i1, i2, opposite] : tet_faces(idx)) {
        if (opposite != GHOST) {
            continue;
        }

        const double volume = signed_volume(
            mesh.vertices[i0],
            mesh.vertices[i1],
            mesh.vertices[i2],
            mesh.vertices[i]
        );

        return volume > EPS;
    }

    tlog::warning("conflicts(): invalid ghost tet. (tid: {})", tid);
    return false;
}

void DelaunayBuilder::insert_vertex(Index i, std::size_t tid) {
    if (tid >= tets.size()) {
        tlog::warning("insert_vertex(): invalid tid. (tid: {})", tid);
        return;
    }
    if (not tets[tid].alive) {
        tlog::warning("insert_vertex(): tet is not alive. (tid: {})", tid);
        return;
    }

    const auto faces = tet_faces(tets[tid].idx);

    del_tet(tid);

    for (const auto& [i0, i1, i2, opposite] : faces) {
        dig_cavity(i, i2, i1, i0);
    }
}

void DelaunayBuilder::dig_cavity(Index i, Index i0, Index i1, Index i2) {
    const auto adj = adjacent(i0, i1, i2);
    if (not adj) {
        return;
    }

    const auto [tid, i3] = *adj;

    if (conflicts(i, tid)) {
        const auto faces = tet_faces(tets[tid].idx);
        del_tet(tid);
        for (const auto& [j0, j1, j2, opposite] : faces) {
            dig_cavity(i, j2, j1, j0);
        }
        return;
    }

    add_tet(i, i0, i1, i2);
}

void DelaunayBuilder::add_tet(Index i0, Index i1, Index i2, Index i3) {
    std::array<Index, 4> idx{i0, i1, i2, i3};

    if (not is_ghost(idx)) {
        const double volume = signed_volume(
            mesh.vertices[idx[0]],
            mesh.vertices[idx[1]],
            mesh.vertices[idx[2]],
            mesh.vertices[idx[3]]
        );

        if (std::abs(volume) <= EPS) {
            return;
        }

        if (volume < 0.0) {
            std::swap(idx[0], idx[1]);
        }
    }

    std::size_t tid = tets.size();
    tets.push_back(Tet{idx, true});
    record_faces(tid);
}

void DelaunayBuilder::del_tet(std::size_t tid) {
    if (tid >= tets.size()) {
        tlog::warning("del_tet(): invalid tid. (tid: {})", tid);
        return;
    }
    if (not tets[tid].alive) {
        return;
    }
    for (const auto& [i0, i1, i2, opposite] : tet_faces(tets[tid].idx)) {
        const Face face = make_face(i0, i1, i2);
        const auto it = f2tet.find(face);
        if (it == std::end(f2tet)) {
            tlog::warning("del_tet(): missing face. (face: ({},{},{}))", i0, i1, i2);
            continue;
        }
        f2tet.erase(it);
    };
    tets[tid].alive = false;
}

void DelaunayBuilder::record_faces(std::size_t tid) {
    for (const auto& [i0, i1, i2, opposite] : tet_faces(tets[tid].idx)) {
        const Face face = make_face(i0, i1, i2);
        const auto [it, inserted] = f2tet.emplace(face, std::make_pair(tid, opposite));
        if (not inserted) {
            tlog::warning("record_faces(): same face added twice. (tid: {}, face: ({},{},{}))", tid, i0, i1, i2);
        }
    }
}

std::optional<std::pair<std::size_t, Index>> DelaunayBuilder::adjacent(Index i0, Index i1, Index i2) const {
    const auto it = f2tet.find(make_face(i0, i1, i2));
    if (it == std::end(f2tet)) {
        return std::nullopt;
    }
    return it->second;
}

int DelaunayBuilder::in_sphere(Index i, Index i0, Index i1, Index i2, Index i3) const {
    return ::in_sphere(
        mesh.vertices[i],
        mesh.vertices[i0],
        mesh.vertices[i1],
        mesh.vertices[i2],
        mesh.vertices[i3]
    );
}