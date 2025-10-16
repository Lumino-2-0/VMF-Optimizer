#include "VMFParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <regex>
#include <stdexcept>

Vec3 VMFParser::ParseVec3(const std::string& str) {
    Vec3 v{};
    sscanf_s(str.c_str(), "(%lf %lf %lf)", &v.x, &v.y, &v.z);
    return v;
}

std::vector<Brush> VMFParser::ParseVMF(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Failed to open VMF file");

    std::vector<Brush> brushes;
    std::string line;
    int brushCounter = 0;
    int faceCounter = 0;

    Brush currentBrush;
    bool inSolid = false;
    bool inSide = false;
    Face currentFace;
    std::string planeStr, materialStr;

    while (std::getline(file, line)) {
        // Supprimer espaces en début/fin
        line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");

        if (line == "solid") {
            inSolid = true;
            currentBrush = Brush();
            currentBrush.id = brushCounter++;
        }
        else if (inSolid && line == "}") {
            // Fin du solid
            currentBrush.ComputeAABB();
            brushes.push_back(currentBrush);
            inSolid = false;
        }

        if (inSolid && line == "side") {
            inSide = true;
            currentFace = Face();
            currentFace.id = faceCounter++;
            currentFace.brushID = currentBrush.id;
            planeStr = "";
            materialStr = "";
        }
        else if (inSide && line == "}") {
            // Fin du side → ajouter la face
            // Parse plane en 3 points
            std::regex planeRegex(R"(\(\s*([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s*\)\s*\(\s*([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s*\)\s*\(\s*([-\d\.]+)\s+([-\d\.]+)\s+([-\d\.]+)\s*\))");
            std::smatch match;
            if (std::regex_search(planeStr, match, planeRegex)) {
                currentFace.p1 = { std::stod(match[1]), std::stod(match[2]), std::stod(match[3]) };
                currentFace.p2 = { std::stod(match[4]), std::stod(match[5]), std::stod(match[6]) };
                currentFace.p3 = { std::stod(match[7]), std::stod(match[8]), std::stod(match[9]) };

                currentFace.center.x = (currentFace.p1.x + currentFace.p2.x + currentFace.p3.x) / 3.0;
                currentFace.center.y = (currentFace.p1.y + currentFace.p2.y + currentFace.p3.y) / 3.0;
                currentFace.center.z = (currentFace.p1.z + currentFace.p2.z + currentFace.p3.z) / 3.0;
            }

            currentFace.material = materialStr;
            currentBrush.faces.push_back(currentFace);
            inSide = false;
        }

        // Lire le plane
        if (inSide && line.find("\"plane\"") != std::string::npos) {
            size_t firstQuote = line.find('\"', 7);
            size_t lastQuote = line.rfind('\"');
            if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                planeStr = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
            }
        }

        // Lire le material
        if (inSide && line.find("\"material\"") != std::string::npos) {
            size_t firstQuote = line.find('\"', 10);
            size_t lastQuote = line.rfind('\"');
            if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                materialStr = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
            }
        }
    }

    return brushes;
}
