#include "Visibility.h"
#include <cmath>
#include <iostream>


// subtract
inline Vec3 Sub(const Vec3& a, const Vec3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

// compute normalized normal of a triangular face
static Vec3 ComputeNormal(const Face& f) {
    Vec3 u = Sub(f.p2, f.p1);
    Vec3 v = Sub(f.p3, f.p1);
    Vec3 n = {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    };
    double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0.0) {
        n.x /= len; n.y /= len; n.z /= len;
    }
    return n;
}

// check if normals are approximately opposite
static bool NormalsOpposite(const Vec3& n1, const Vec3& n2, double tol = 1e-2) {
    double d = Dot(n1, n2);
    return d <= (-1.0 + tol);
}

// check if point p lies on plane defined by p0 and normal n (within eps)
static bool PointOnPlane(const Vec3& p, const Vec3& p0, const Vec3& n, double eps = 0.01) {
    Vec3 v = Sub(p, p0);
    double dist = Dot(v, n);
    return std::abs(dist) <= eps;
}

// check if two faces are roughly coplanar (all vertices of f2 lie on f1's plane)
static bool FacesCoplanar(const Face& f1, const Face& f2, double eps = 0.01) {
    return PointOnPlane(f2.p1, f1.p1, f1.normal, eps) &&
        PointOnPlane(f2.p2, f1.p1, f1.normal, eps) &&
        PointOnPlane(f2.p3, f1.p1, f1.normal, eps);
}

// triangle area
static double FaceArea(const Face& f) {
    Vec3 v1 = Sub(f.p2, f.p1);
    Vec3 v2 = Sub(f.p3, f.p1);
    Vec3 c = {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };
    return 0.5 * std::sqrt(c.x * c.x + c.y * c.y + c.z * c.z);
}

std::vector<Face*> Visibility::DetectHiddenFaces(std::vector<Brush>& brushes) {
    std::vector<Face*> hiddenFaces;
    int totalFaces = 0;

    // ensure face derived data (center, normal) present and compute AABB
    for (auto& b : brushes) {
        for (auto& f : b.faces) {
            // if parser didn't compute normal/center, compute now
            f.ComputeDerived();
            ++totalFaces;
        }
        b.ComputeAABB();
    }

    // pairwise comparison between brushes' faces (fast reject using AABB)
    for (size_t i = 0; i < brushes.size(); ++i) {
        for (size_t j = 0; j < brushes.size(); ++j) {
            if (i == j) continue;
            Brush& A = brushes[i];
            Brush& B = brushes[j];

            // quick AABB overlap test; skip if boxes far apart
            if (A.max.x < B.min.x || A.min.x > B.max.x ||
                A.max.y < B.min.y || A.min.y > B.max.y ||
                A.max.z < B.min.z || A.min.z > B.max.z) {
                continue;
            }

            for (auto& fa : A.faces) {
                for (auto& fb : B.faces) {
                    // if already hidden both, skip small work
                    if (fa.hidden && fb.hidden) continue;

                    // compare normals: must be roughly opposite
                    if (!NormalsOpposite(fa.normal, fb.normal)) continue;

                    // faces must be coplanar (approx)
                    if (!FacesCoplanar(fa, fb)) continue;

                    // now decide by area: equal -> both hidden, else smaller hidden
                    double aA = FaceArea(fa);
                    double aB = FaceArea(fb);

                    const double AREA_EPS = 1e-3;
                    if (std::abs(aA - aB) <= AREA_EPS) {
                        if (!fa.hidden) { fa.hidden = true; hiddenFaces.push_back(&fa); }
                        if (!fb.hidden) { fb.hidden = true; hiddenFaces.push_back(&fb); }
                    }
                    else if (aA < aB) {
                        if (!fa.hidden) { fa.hidden = true; hiddenFaces.push_back(&fa); }
                    }
                    else {
                        if (!fb.hidden) { fb.hidden = true; hiddenFaces.push_back(&fb); }
                    }
                }
            }
        }
    }

    std::cout << "Total faces: " << totalFaces << "\n";
    std::cout << "Detected " << hiddenFaces.size() << " hidden faces.\n";
    return hiddenFaces;
}
