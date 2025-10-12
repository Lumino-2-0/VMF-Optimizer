#pragma once
#include <vector>
#include <string>

struct Vec3 {
    double x, y, z;
};

struct Face {
    int id;          // ID unique
    int brushID;     // ID du brush parent
    Vec3 p1, p2, p3; // Trois points pour la face
    Vec3 center;     // Centre de la face
    std::string material; // Nom de la texture appliquée
};

struct Brush {
    int id;
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
