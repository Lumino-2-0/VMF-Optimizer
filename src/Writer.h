#pragma once
#include <vector>
#include <string>

class Writer {
public:
    static void ApplyNodraw(const std::string& inputPath,
        const std::string& outputPath,
        const std::vector<int>& hiddenFaces);
};
