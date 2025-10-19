#include "VMFParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <regex>
#include <stdexcept>

// Parse a string like "(12 34 56)" into Vec3
Vec3 VMFParser::ParseVec3(const std::string& str) {
    Vec3 v{};
    // accept ints or floats, negatives
    sscanf_s(str.c_str(), "(%lf %lf %lf)", &v.x, &v.y, &v.z);
    return v;
}

std::vector<Brush> VMFParser::ParseVMF(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Failed to open VMF file: " + path);

    std::vector<Brush> brushes;

    std::string rawLine;
    int brushCounter = 0;
    int faceCounter = 0;

    // States
    bool pendingSolid = false;   // we've seen "solid" but not yet its opening '{'
    bool inSolid = false;        // currently inside a solid { ... }
    int solidBraceDepth = 0;     // brace depth inside the current solid (counts nested braces)

    bool pendingSide = false;    // we've seen "side" but not yet its opening '{'
    bool inSide = false;         // currently inside a side { ... }
    int sideBraceDepth = 0;      // brace depth inside the current side block

    Brush currentBrush;
    Face  currentFace;

    std::ostringstream sideBuffer; // accumulate lines inside a side block

    auto trim = [](std::string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) { s.clear(); return; }
        size_t b = s.find_last_not_of(" \t\r\n");
        s = s.substr(a, b - a + 1);
        };

    // regex to capture vectors like "(x y z)" (integers or floats, negative allowed)
    std::regex vecRegex(R"(\(\s*([+-]?\d+\.?\d*)\s+([+-]?\d+\.?\d*)\s+([+-]?\d+\.?\d*)\s*\))");

    while (std::getline(file, rawLine)) {
        std::string line = rawLine;
        trim(line);
        if (line.empty()) continue;

        // Detect tokens even if '{' or other tokens are on separate lines.
        // Handle solid start
        if (!inSolid && !pendingSolid) {
            // "solid" token (may be "solid" alone or "solid <something>")
            if (line.size() >= 5 && line.rfind("solid", 0) == 0) {
                pendingSolid = true;
                // don't create brush yet, wait for opening brace
                continue;
            }
        }

        // If pendingSolid and we hit an opening brace, we officially enter the solid
        if (pendingSolid && line.find("{") != std::string::npos) {
            pendingSolid = false;
            inSolid = true;
            solidBraceDepth = 1; // opened the solid block
            currentBrush = Brush();
            currentBrush.id = brushCounter++;
            continue;
        }

        // If already inSolid, we must track braces and detect side blocks and end of solid
        if (inSolid) {
            // Opening brace inside solid increases depth
            if (line.find("{") != std::string::npos) {
                // If we're starting a side block and the '{' belongs to side, we'll handle below.
                // But keep generic: update depth
                solidBraceDepth++;
            }
            // Closing brace reduces depth; if it matches solid's root, close solid.
            if (line.find("}") != std::string::npos) {
                solidBraceDepth--;
                // If we were in a side and encountered a '}', it might end the side — but handled in side flow.
                if (!inSide && solidBraceDepth == 0) {
                    // end of solid
                    if (!currentBrush.faces.empty()) currentBrush.ComputeAABB();
                    brushes.push_back(currentBrush);
                    inSolid = false;
                }
                // continue processing (we still may have other tokens on the same line, but typically not)
            }

            // Detect side start even if '{' on next line
            if (!inSide && !pendingSide) {
                if (line.size() >= 4 && line.rfind("side", 0) == 0) {
                    pendingSide = true;
                    // Wait for its '{' to actually begin side content
                    // but a following "{" may be on the same line or next lines
                    // do not continue here, need to handle same-line '{' below
                }
            }

            // If pendingSide and the current line contains '{', start the side
            if (pendingSide && line.find("{") != std::string::npos) {
                pendingSide = false;
                inSide = true;
                sideBraceDepth = 1;
                currentFace = Face();
                currentFace.id = faceCounter++;
                currentFace.brushID = currentBrush.id;
                sideBuffer.str("");
                sideBuffer.clear();
                // If there is additional content on the same line after '{', ignore (rare)
                continue;
            }

            // If we were not pending side but line contains "side" and "{" on same line (e.g., "side {")
            if (!inSide && line.find("side") != std::string::npos && line.find("{") != std::string::npos) {
                inSide = true;
                sideBraceDepth = 1;
                currentFace = Face();
                currentFace.id = faceCounter++;
                currentFace.brushID = currentBrush.id;
                sideBuffer.str("");
                sideBuffer.clear();
                continue;
            }

            // If we're inside a side, accumulate its content and watch for its braces
            if (inSide) {
                // If the line contains a '{' increase nested depth for the side block
                if (line.find("{") != std::string::npos) {
                    sideBraceDepth++;
                }
                // If the line contains a '}', decrease depth and maybe end side
                if (line.find("}") != std::string::npos) {
                    sideBraceDepth--;
                    if (sideBraceDepth == 0) {
                        // end of side block -> parse accumulated content
                        std::string sideText = sideBuffer.str();
                        std::istringstream ss(sideText);
                        std::string sline;
                        while (std::getline(ss, sline)) {
                            trim(sline);
                            if (sline.empty()) continue;

                            // parse plane lines
                            if (sline.find("\"plane\"") != std::string::npos) {
                                std::vector<Vec3> verts;
                                for (std::sregex_iterator it(sline.begin(), sline.end(), vecRegex), end; it != end; ++it) {
                                    std::smatch m = *it;
                                    Vec3 v{};
                                    v.x = std::stod(m[1].str());
                                    v.y = std::stod(m[2].str());
                                    v.z = std::stod(m[3].str());
                                    verts.push_back(v);
                                }
                                if (verts.size() >= 3) {
                                    currentFace.p1 = verts[0];
                                    currentFace.p2 = verts[1];
                                    currentFace.p3 = verts[2];
                                    currentFace.ComputeDerived();
                                }
                                continue;
                            }

                            // parse material lines
                            if (sline.find("\"material\"") != std::string::npos) {
                                size_t lastQ = sline.rfind('\"');
                                if (lastQ != std::string::npos) {
                                    size_t prevQ = sline.rfind('\"', lastQ - 1);
                                    if (prevQ != std::string::npos && lastQ > prevQ) {
                                        currentFace.material = sline.substr(prevQ + 1, lastQ - prevQ - 1);
                                    }
                                }
                                continue;
                            }

                            // other side-level keys are ignored for parsing faces
                        }

                        // push face into current brush (even if plane was not found, face object kept)
                        currentBrush.faces.push_back(currentFace);
                        inSide = false;
                        continue;
                    }
                }

                // if still inside side, append the line to buffer (except the closing brace which we've handled)
                if (inSide) {
                    sideBuffer << rawLine << "\n"; // keep rawLine to preserve formatting if needed later
                }
                continue;
            } // end inSide handling

            // Other tokens inside solid that we do not need (id, editor, vertices_plus etc.) are ignored here
            continue;
        } // end inSolid

        // If not in solid and not pendingSolid, but line contains '{' or '}' standalone for other sections, ignore
    } // end while lines

    // End of file: if a solid is still open, finalize it
    if (inSide) {
        // finalize last side if file ended unexpectedly
        currentBrush.faces.push_back(currentFace);
        inSide = false;
    }
    if (inSolid) {
        if (!currentBrush.faces.empty()) currentBrush.ComputeAABB();
        brushes.push_back(currentBrush);
        inSolid = false;
    }

    // Final stats
    size_t totalFaces = 0;
    for (auto& b : brushes) totalFaces += b.faces.size();

    std::cout << "Total brushes parsed: " << brushes.size() << "\n";
    std::cout << "Total faces parsed: " << totalFaces << "\n";

    return brushes;
}
