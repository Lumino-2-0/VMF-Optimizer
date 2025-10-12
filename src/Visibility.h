#pragma once
#include "Geometry.h"
#include <vector>

class Visibility {
public:
    static std::vector<int> GetHiddenFaces(std::vector<Brush>& brushes);

private:
    static bool IsFaceVisible(const Face& face, const std::vector<Brush>& brushes);
    static bool IsPointOccluded(const Vec3& point, const Brush& brush);
};
