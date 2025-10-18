#pragma once
#include "Geometry.h"
#include <string>
#include <vector>

class VMFParser {
public:
    // Parse the given VMF and return brushes + faces
    static std::vector<Brush> ParseVMF(const std::string& path);

private:
    // helper: parse "(x y z)" into Vec3
    static Vec3 ParseVec3(const std::string& str);
};
