#include "VMFParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

Vec3 VMFParser::ParseVec3(const std::string& str) {
    Vec3 v{};
    sscanf_s(str.c_str(), "(%lf %lf %lf)", &v.x, &v.y, &v.z);
    return v;
}

std::vector<Brush> VMFParser::ParseVMF(const std::string& path) {
    std::vector<Brush> brushes;
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Failed to open VMF file");

    std::string line;
    int brushCounter = 0;
    int faceCounter = 0;
    Brush currentBrush;
    bool inBrush = false;

    while (std::getline(file, line)) {
        if (line.find("brush") != std::string::npos && line.find("{") != std::string::npos) {
            inBrush = true;
            currentBrush = Brush();
            currentBrush.id = brushCounter++;
        }
        else if (inBrush && line.find("}") != std::string::npos) {
            currentBrush.ComputeAABB();
            brushes.push_back(currentBrush);
            inBrush = false;
        }

        // Détection simple des faces
        std::regex faceRegex(R"(\(\s*([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s*\)\s*\(\s*([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s*\)\s*\(\s*([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s*\)\s*\"([^\"]+)\")");
        std::smatch match;
        if (std::regex_search(line, match, faceRegex)) {
            Face f;
            f.id = faceCounter++;
            f.brushID = currentBrush.id;

            f.p1 = { std::stod(match[1]), std::stod(match[2]), std::stod(match[3]) };
            f.p2 = { std::stod(match[4]), std::stod(match[5]), std::stod(match[6]) };
            f.p3 = { std::stod(match[7]), std::stod(match[8]), std::stod(match[9]) };
            f.center.x = (f.p1.x + f.p2.x + f.p3.x) / 3.0;
            f.center.y = (f.p1.y + f.p2.y + f.p3.y) / 3.0;
            f.center.z = (f.p1.z + f.p2.z + f.p3.z) / 3.0;

            f.material = match[10];

            currentBrush.faces.push_back(f);
        }
    }

    return brushes;
}
