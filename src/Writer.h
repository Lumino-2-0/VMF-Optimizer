#pragma once
#include "Geometry.h"
#include <string>
#include <vector>

class Writer {
public:
    // inputPath: original VMF file
    // outputPath: target VMF file
    // brushes: parsed brushes with face.hidden flags set
    static void ApplyNodraw(const std::string& inputPath,
        const std::string& outputPath,
        const std::vector<Brush>& brushes);
};
