#include "Visibility.h"
#include <iostream>

bool Visibility::IsPointOccluded(const Vec3& point, const Brush& brush) {
    return (point.x >= brush.min.x && point.x <= brush.max.x &&
            point.y >= brush.min.y && point.y <= brush.max.y &&
            point.z >= brush.min.z && point.z <= brush.max.z);
}

bool Visibility::IsFaceVisible(const Face& face, const std::vector<Brush>& brushes) {
    Vec3 c = face.center;
    for (const auto& brush : brushes) {
        if (brush.id == face.brushID) continue;
        if (IsPointOccluded(c, brush)) return false;
    }
    return true;
}

std::vector<int> Visibility::GetHiddenFaces(std::vector<Brush>& brushes) {
    std::vector<int> hiddenFaces;

    for (auto& brush : brushes) brush.ComputeAABB();

    for (const auto& brush : brushes) {
        for (const auto& face : brush.faces) {
            if (!IsFaceVisible(face, brushes)) hiddenFaces.push_back(face.id);
        }
    }

    std::cout << "Detected " << hiddenFaces.size() << " hidden faces.\n";
    return hiddenFaces;
}
