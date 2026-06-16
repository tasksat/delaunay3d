#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "delaunay/delaunay.hpp"

struct Tet {
    std::array<Index, 4> idx{};
    bool alive = true;
};

struct Face {
    std::array<Index, 3> idx{};
    bool operator==(const Face&) const = default;
};

// hash mix inspired by Boost.ContainerHash
struct FaceHash {
    std::size_t operator()(const Face& face) const noexcept {
        std::uint32_t h = 0;
        for (Index i : face.idx) {
            h = mix(h + 0x9e3779b9 + i);
        }
        return static_cast<std::size_t>(h);
    }

  private:
    // https://github.com/skeeto/hash-prospector/issues/19
    static std::uint32_t mix(std::uint32_t x) noexcept {
        constexpr std::uint32_t m1 = 0x21f0aaad;
        constexpr std::uint32_t m2 = 0x735a2d97;
        x ^= x >> 16;
        x *= m1;
        x ^= x >> 15;
        x *= m2;
        x ^= x >> 15;
        return x;
    }
};

class DelaunayBuilder {
  public:
    explicit DelaunayBuilder(std::span<const Vec3> points);

    Tetrahedra run();

    void add_point(const Vec3& p);

  private:
    std::optional<std::array<Index, 4>> initialize();

    Tetrahedra export_mesh() const;

    // Bowyer-Watson
    std::optional<std::size_t> find_conflict(Index i) const;
    bool conflicts(Index i, std::size_t tid) const;
    void insert_vertex(Index i, std::size_t tid);
    void dig_cavity(Index i, Index i0, Index i1, Index i2);

    void add_tet(Index i0, Index i1, Index i2, Index i3);
    void del_tet(std::size_t tid);

    void record_faces(std::size_t tid);

    std::optional<std::pair<std::size_t, Index>> adjacent(Index i0, Index i1, Index i2) const;
    int in_sphere(Index i, Index i0, Index i1, Index i2, Index i3) const;

    Tetrahedra mesh;
    std::vector<Tet> tets;
    std::unordered_map<Face, std::pair<std::size_t, Index>, FaceHash> f2tet;
};