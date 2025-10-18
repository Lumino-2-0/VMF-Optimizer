#include "Writer.h"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

// We assume the parser produced faces in the same order as side blocks in the file.
// This function reads the VMF and replaces the "material" value inside side blocks
// when the corresponding face has hidden==true. It iterates faces in the same sequence.
void Writer::ApplyNodraw(const std::string& inputPath,
    const std::string& outputPath,
    const std::vector<Brush>& brushes) {
    std::ifstream in(inputPath);
    if (!in.is_open()) {
        std::cerr << "Writer: failed to open input VMF: " << inputPath << "\n";
        return;
    }
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        std::cerr << "Writer: failed to open output VMF: " << outputPath << "\n";
        return;
    }

    std::string rawLine;
    bool inSide = false;
    // Flatten faces in the same order parser produced them
    std::vector<const Face*> allFaces;
    for (const auto& b : brushes) {
        for (const auto& f : b.faces) allFaces.push_back(&f);
    }
    size_t faceIndex = 0;

    while (std::getline(in, rawLine)) {
        std::string line = rawLine;
        // trim left for easier checks (but keep rawLine for output formatting)
        size_t pos = line.find_first_not_of(" \t\r\n");
        std::string trimmed = (pos == std::string::npos) ? "" : line.substr(pos);

        if (!inSide) {
            if (trimmed == "side" || trimmed == "side{") {
                inSide = true;
                out << rawLine << "\n";
                continue;
            }
            else {
                out << rawLine << "\n";
                continue;
            }
        }
        else {
            // we are inside a side block
            if (trimmed.find("\"material\"") != std::string::npos) {
                // if faceIndex is valid and face is hidden -> replace the material in this line
                if (faceIndex < allFaces.size() && allFaces[faceIndex]->hidden) {
                    // find first quote after material token and replace the content between the quotes
                    size_t firstQuote = rawLine.find('"');
                    if (firstQuote != std::string::npos) {
                        // find the next two quotes: skip token name "material"
                        size_t second = rawLine.find('"', firstQuote + 1);
                        size_t third = rawLine.find('"', second + 1);
                        size_t fourth = rawLine.find('"', third + 1);
                        if (third != std::string::npos && fourth != std::string::npos && fourth > third) {
                            std::string newLine = rawLine;
                            newLine.replace(third + 1, fourth - third - 1, "tools/nodraw");
                            out << newLine << "\n";
                        }
                        else {
                            // fallback: write a standard material line replacing whole line
                            out << "\t\"material\" \"tools/nodraw\"\n";
                        }
                    }
                    else {
                        out << "\t\"material\" \"tools/nodraw\"\n";
                    }
                }
                else {
                    // not hidden -> copy as is
                    out << rawLine << "\n";
                }
                continue;
            }

            // end of side block
            if (trimmed == "}") {
                inSide = false;
                // increment face index after finishing a side block
                ++faceIndex;
                out << rawLine << "\n";
                continue;
            }

            // other lines inside side
            out << rawLine << "\n";
            continue;
        }
    }

    in.close();
    out.close();
    std::cout << "Optimized VMF written to " << outputPath << "\n";
}
