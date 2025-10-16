#pragma once
#include "VMFParser.h"
#include <vector>

class Visibility {
public:
    static void DetectHiddenFaces(std::vector<Brush>& brushes);
};

class Visibility {
public:
    static std::vector<int> GetHiddenFaces(std::vector<Brush>& brushes);

private:
    static bool IsFaceVisible(const Face& face, const std::vector<Brush>& brushes);
    static bool IsPointOccluded(const Vec3& point, const Brush& brush);
};

class Face {
public:
    int id;
    int brushID;
    Vec3 p1, p2, p3;
    Vec3 center;
    std::string material;
    bool hidden = false; // ← nouvelle variable
};
