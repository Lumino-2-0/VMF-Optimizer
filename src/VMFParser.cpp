#include "VMFParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <stdexcept>

Vec3 VMFParser::ParseVec3(const std::string& str) {
    Vec3 v{};
    sscanf_s(str.c_str(), "(%lf %lf %lf)", &v.x, &v.y, &v.z);
    return v;
}

std::vector<Brush> VMFParser::ParseVMF(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Failed to open VMF file: " + path);

    std::vector<Brush> brushes;
    std::string line;
    Brush currentBrush;
    Face currentFace;
    bool inSolid = false;
    bool inSide = false;
    int brushCounter = 0;
    int faceCounter = 0;

    while (std::getline(file, line)) {
        // trim
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        // Start of a solid
        if (line.find("solid") != std::string::npos && line.find("{") != std::string::npos) {
            inSolid = true;
            currentBrush = Brush();
            currentBrush.id = brushCounter++;
            continue;
        }

        // Start of a face
        if (inSolid && line.find("side") != std::string::npos && line.find("{") != std::string::npos) {
            inSide = true;
            currentFace = Face();
            currentFace.id = faceCounter++;
            currentFace.brushID = currentBrush.id;
            continue;
        }

        // Parse inside face
        if (inSide) {
            if (line.find("\"plane\"") != std::string::npos) {
                size_t p1 = line.find('(');
                size_t p2 = line.rfind(')');
                if (p1 != std::string::npos && p2 != std::string::npos && p2 > p1) {
                    std::string planeStr = line.substr(p1, p2 - p1 + 1);
                    size_t pos = 0;
                    auto extract = [&](size_t& pos) -> Vec3 {
                        size_t open = planeStr.find('(', pos);
                        size_t close = planeStr.find(')', open);
                        pos = close + 1;
                        return ParseVec3(planeStr.substr(open, close - open + 1));
                    };
                    currentFace.p1 = extract(pos);
                    currentFace.p2 = extract(pos);
                    currentFace.p3 = extract(pos);
                    currentFace.ComputeDerived();
                }
                continue;
            }
            if (line.find("\"material\"") != std::string::npos) {
                size_t lastQuote = line.rfind('\"');
                size_t prevQuote = line.rfind('\"', lastQuote - 1);
                if (prevQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > prevQuote) {
                    currentFace.material = line.substr(prevQuote + 1, lastQuote - prevQuote - 1);
                }
                continue;
            }
            if (line.find("}") != std::string::npos) {
                // end of side
                currentBrush.faces.push_back(currentFace);
                inSide = false;
                continue;
            }
        }

        // End of a brush
        if (inSolid && !inSide && line.find("}") != std::string::npos) {
            currentBrush.ComputeAABB();
            brushes.push_back(currentBrush);
            inSolid = false;
            continue;
        }
    }

    std::cout << "Total faces: ";
    size_t totalFaces = 0;
    for (auto& b : brushes) totalFaces += b.faces.size();
    std::cout << totalFaces << "\n";

    return brushes;
}
