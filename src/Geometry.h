#pragma once
#include <vector>
#include <string>
#include <cmath>

struct Vec3 {
    double x = 0;
    double y = 0;
    double z = 0;
    Vec3() = default;
    Vec3(double X, double Y, double Z) : x(X), y(Y), z(Z) {}

    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator*(double s) const { return { x * s, y * s, z * s }; }
};

inline double Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}
inline double Length(const Vec3& a) {
    return std::sqrt(Dot(a, a));
}
inline Vec3 Normalize(const Vec3& a) {
    double L = Length(a);
    if (L == 0) return { 0,0,0 };
    return { a.x / L, a.y / L, a.z / L };
}

struct Face {
    int id = -1;            // unique face id (sequence)
    int brushID = -1;       // parent brush id
    Vec3 p1, p2, p3;        // three points defining the face (plane)
    Vec3 center;            // computed center
    Vec3 normal = { 0,0,0 };  // unit normal pointing outwards (computed)
    std::string material;   // texture name
    bool hidden = false;    // set by visibility pass

    // compute center and normal (call after p1,p2,p3 are set)
    void ComputeDerived() {
        center.x = (p1.x + p2.x + p3.x) / 3.0;
        center.y = (p1.y + p2.y + p3.y) / 3.0;
        center.z = (p1.z + p2.z + p3.z) / 3.0;
        Vec3 v1 = p2 - p1;
        Vec3 v2 = p3 - p1;
        normal = Normalize(Cross(v1, v2));
    }
};

struct Brush {
    int id = -1;
    std::vector<Face> faces;
    Vec3 min;
    Vec3 max;

    void ComputeAABB() {
        if (faces.empty()) return;
        min = max = faces[0].center;
        for (const auto& f : faces) {
            if (f.center.x < min.x) min.x = f.center.x;
            if (f.center.y < min.y) min.y = f.center.y;
            if (f.center.z < min.z) min.z = f.center.z;

            if (f.center.x > max.x) max.x = f.center.x;
            if (f.center.y > max.y) max.y = f.center.y;
            if (f.center.z > max.z) max.z = f.center.z;
        }
    }
};
