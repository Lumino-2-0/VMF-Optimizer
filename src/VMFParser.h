#pragma once
#include "Geometry.h"
#include <string>
#include <vector>

class VMFParser {
public:
    static std::vector<Brush> ParseVMF(const std::string& path);

private:
    static Vec3 ParseVec3(const std::string& str); // convertit "(x y z)" en Vec3
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
