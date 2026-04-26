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
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline double Length(const Vec3& a) {
    return std::sqrt(Dot(a, a));
}

inline Vec3 Normalize(const Vec3& a) {
    double L = Length(a);
    if (L == 0.0) return { 0, 0, 0 };
    return { a.x / L, a.y / L, a.z / L };
}

struct Face {
    int id = -1;
    int brushID = -1;

    // Legacy support (kept for compatibility with parser)
    Vec3 p1, p2, p3;

    // New system (true polygon support)
    std::vector<Vec3> vertices;

    Vec3 center;
    Vec3 normal = { 0, 0, 0 };

    Vec3 min;
    Vec3 max;

    std::string material;
    bool hidden = false;

    void ComputeDerived()
    {
        center = { 0, 0, 0 };

        if (!vertices.empty())
        {
            for (const Vec3& v : vertices)
            {
                center = center + v;
            }

            double inv = 1.0 / vertices.size();
            center = center * inv;
        }
        else
        {
            center = (p1 + p2 + p3) * (1.0 / 3.0);
        }

        if (vertices.size() >= 3)
        {
            Vec3 v1 = vertices[1] - vertices[0];
            Vec3 v2 = vertices[2] - vertices[0];
            normal = Normalize(Cross(v1, v2));
        }
        else
        {
            Vec3 v1 = p2 - p1;
            Vec3 v2 = p3 - p1;
            normal = Normalize(Cross(v1, v2));
        }
    }

    void ComputeBounds()
    {
        if (vertices.empty())
        {
            min = max = p1;

            if (p2.x < min.x) min.x = p2.x;
            if (p2.y < min.y) min.y = p2.y;
            if (p2.z < min.z) min.z = p2.z;

            if (p2.x > max.x) max.x = p2.x;
            if (p2.y > max.y) max.y = p2.y;
            if (p2.z > max.z) max.z = p2.z;

            if (p3.x < min.x) min.x = p3.x;
            if (p3.y < min.y) min.y = p3.y;
            if (p3.z < min.z) min.z = p3.z;

            if (p3.x > max.x) max.x = p3.x;
            if (p3.y > max.y) max.y = p3.y;
            if (p3.z > max.z) max.z = p3.z;

            return;
        }

        min = max = vertices[0];

        for (const Vec3& v : vertices)
        {
            if (v.x < min.x) min.x = v.x;
            if (v.y < min.y) min.y = v.y;
            if (v.z < min.z) min.z = v.z;

            if (v.x > max.x) max.x = v.x;
            if (v.y > max.y) max.y = v.y;
            if (v.z > max.z) max.z = v.z;
        }
    }
};

struct Brush {
    int id = -1;
    std::vector<Face> faces;

    Vec3 min;
    Vec3 max;

    void ComputeAABB()
    {
        if (faces.empty()) return;

        bool first = true;

        for (const auto& f : faces)
        {
            for (const Vec3& v : f.vertices)
            {
                if (first)
                {
                    min = max = v;
                    first = false;
                    continue;
                }

                if (v.x < min.x) min.x = v.x;
                if (v.y < min.y) min.y = v.y;
                if (v.z < min.z) min.z = v.z;

                if (v.x > max.x) max.x = v.x;
                if (v.y > max.y) max.y = v.y;
                if (v.z > max.z) max.z = v.z;
            }

            // fallback if no vertices
            if (f.vertices.empty())
            {
                const Vec3& c = f.center;

                if (first)
                {
                    min = max = c;
                    first = false;
                }
                else
                {
                    if (c.x < min.x) min.x = c.x;
                    if (c.y < min.y) min.y = c.y;
                    if (c.z < min.z) min.z = c.z;

                    if (c.x > max.x) max.x = c.x;
                    if (c.y > max.y) max.y = c.y;
                    if (c.z > max.z) max.z = c.z;
                }
            }
        }
    }
};
