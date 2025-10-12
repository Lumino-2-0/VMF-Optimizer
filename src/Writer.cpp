#include "Writer.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void Writer::ApplyNodraw(const std::string& inputPath,
    const std::string& outputPath,
    const std::vector<int>& hiddenFaces) {
    std::ifstream file(inputPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open input VMF.\n";
        return;
    }

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open output VMF.\n";
        return;
    }

    std::string line;
    int currentFaceID = 0;
    while (std::getline(file, line)) {
        if (!hiddenFaces.empty() && std::find(hiddenFaces.begin(), hiddenFaces.end(), currentFaceID) != hiddenFaces.end()) {
            // remplacer le material par "TOOLS/NODRAW"
            size_t quotePos = line.rfind('"');
            if (quotePos != std::string::npos) line.replace(line.rfind('"', quotePos - 1) + 1, quotePos - line.rfind('"', quotePos - 1) - 1, "TOOLS/NODRAW");
        }
        out << line << "\n";
        if (line.find("(") != std::string::npos && line.find(")") != std::string::npos) currentFaceID++;
    }

    std::cout << "Optimized VMF written to " << outputPath << "\n";
}
