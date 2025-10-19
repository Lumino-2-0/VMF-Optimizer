#include "Writer.h"
#include "VMFParser.h" // pour Brush / Face structures
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

// Petite aide pour trim gauche/droite, on garde l'indentation originale
static inline std::string ltrim_copy(const std::string& s) {
    size_t i = s.find_first_not_of(" \t\r\n");
    return (i == std::string::npos) ? std::string() : s.substr(i);
}
static inline std::string rtrim_copy(const std::string& s) {
    size_t i = s.find_last_not_of(" \t\r\n");
    return (i == std::string::npos) ? std::string() : s.substr(0, i + 1);
}
static inline std::string trim_copy(const std::string& s) {
    return rtrim_copy(ltrim_copy(s));
}

// Remplace la valeur d'un keyvalue "material" dans une ligne VMF, en conservant l'indentation.
// Si la ligne ne contient pas de "material", retourne false.
static bool ReplaceMaterialLine(std::string& line, const std::string& newMat) {
    std::string trimmed = trim_copy(line);
    // On veut seulement remplacer une vraie ligne de material (ex:  "material" "blah")
    if (trimmed.size() < 10) return false;
    // On exige que "material" apparaisse en tant que clé
    size_t keyPos = trimmed.find("\"material\"");
    if (keyPos == std::string::npos) return false;

    // Récupérer l'indentation d'origine
    size_t firstNonWs = line.find_first_not_of(" \t\r\n");
    std::string indent = (firstNonWs == std::string::npos) ? "" : line.substr(0, firstNonWs);

    // Construire la nouvelle ligne propre
    std::ostringstream oss;
    oss << indent << "\"material\" \"" << newMat << "\"";
    line = oss.str();
    return true;
}

void Writer::ApplyNodraw(const std::string& srcPath,
    const std::string& dstPath,
    const std::vector<Brush>& brushes)
{
    std::ifstream in(srcPath);
    std::ofstream out(dstPath, std::ios::trunc);

    if (!in.is_open() || !out.is_open()) {
        // On ne throw pas ici pour rester soft, mais on pourrait
        std::cerr << "Writer: failed to open input or output file.\n";
        return;
    }

    // On s'aligne sur l'ordre exact rencontré lors du parsing :
    // - brushes : dans l'ordre où le parser les a rencontrés
    // - faces   : dans l'ordre des 'side' à l'intérieur du 'solid'
    int brushIndex = -1;
    int faceIndexWithinBrush = -1;

    bool pendingSolid = false;
    bool inSolid = false;
    int solidBraceDepth = 0;

    bool pendingSide = false;
    bool inSide = false;
    int sideBraceDepth = 0;

    // Pour savoir si, pendant un side, on a déjà vu une ligne "material"
    bool sideSawMaterialLine = false;

    std::string rawLine;
    while (std::getline(in, rawLine)) {
        std::string line = trim_copy(rawLine);

        // Détection "solid" (même si { est sur la ligne suivante)
        if (!inSolid && !pendingSolid) {
            if (line.rfind("solid", 0) == 0) {
                pendingSolid = true;
                out << rawLine << "\n";
                continue;
            }
        }

        // Ouverture du block de solid
        if (pendingSolid && line.find("{") != std::string::npos) {
            pendingSolid = false;
            inSolid = true;
            solidBraceDepth = 1;
            brushIndex++;
            faceIndexWithinBrush = -1;
            out << rawLine << "\n";
            continue;
        }

        if (inSolid) {
            // Gestion générique des braces pour solid
            if (line.find("{") != std::string::npos) solidBraceDepth++;
            if (line.find("}") != std::string::npos) solidBraceDepth--;

            // Détection "side" (même si { est sur la ligne suivante)
            if (!inSide && !pendingSide) {
                if (line.rfind("side", 0) == 0) {
                    pendingSide = true;
                    sideSawMaterialLine = false;
                    out << rawLine << "\n";
                    continue;
                }
            }

            // Ouverture effective du side
            if (pendingSide && line.find("{") != std::string::npos) {
                pendingSide = false;
                inSide = true;
                sideBraceDepth = 1;
                faceIndexWithinBrush++;
                sideSawMaterialLine = false;
                out << rawLine << "\n";
                continue;
            }

            // Cas "side {" sur la même ligne
            if (!inSide && line.find("side") != std::string::npos && line.find("{") != std::string::npos) {
                inSide = true;
                sideBraceDepth = 1;
                faceIndexWithinBrush++;
                sideSawMaterialLine = false;
                out << rawLine << "\n";
                continue;
            }

            // À l’intérieur d’un side : on remplace la ligne material si la face est cachée
            if (inSide) {
                if (line.find("{") != std::string::npos) sideBraceDepth++;
                if (line.find("}") != std::string::npos) {
                    sideBraceDepth--;
                    // Si on ferme le side, et qu'on n'a pas encore vu de ligne material,
                    // mais que la face est cachée, on injecte une ligne material juste avant la fermeture.
                    if (sideBraceDepth == 0) {
                        // Inject material if needed and not seen
                        if (brushIndex >= 0 && brushIndex < (int)brushes.size()) {
                            const auto& b = brushes[brushIndex];
                            if (faceIndexWithinBrush >= 0 && faceIndexWithinBrush < (int)b.faces.size()) {
                                const auto& f = b.faces[faceIndexWithinBrush];
                                if (f.hidden && !sideSawMaterialLine) {
                                    // On injecte la ligne material avec l'indentation de cette ligne "}".
                                    // On récupère l'indentation actuelle :
                                    size_t firstNonWs = rawLine.find_first_not_of(" \t\r\n");
                                    std::string indent = (firstNonWs == std::string::npos) ? "" : rawLine.substr(0, firstNonWs);
                                    out << indent << "    \"material\" \"tools/toolsnodraw\"\n";
                                }
                            }
                        }

                        // Écrire la ligne de fermeture du side
                        out << rawLine << "\n";
                        inSide = false;
                        continue;
                    }
                }

                // Si c'est une ligne "material", on remplace si la face est cachée
                if (line.find("\"material\"") != std::string::npos) {
                    sideSawMaterialLine = true;
                    // Est-ce que la face courante est cachée ?
                    bool shouldNodraw = false;
                    if (brushIndex >= 0 && brushIndex < (int)brushes.size()) {
                        const auto& b = brushes[brushIndex];
                        if (faceIndexWithinBrush >= 0 && faceIndexWithinBrush < (int)b.faces.size()) {
                            shouldNodraw = b.faces[faceIndexWithinBrush].hidden;
                        }
                    }

                    if (shouldNodraw) {
                        std::string modLine = rawLine;
                        if (!ReplaceMaterialLine(modLine, "tools/toolsnodraw")) {
                            // Si pour une raison quelconque on n'a pas pu remplacer, on injecte une ligne propre.
                            size_t firstNonWs = rawLine.find_first_not_of(" \t\r\n");
                            std::string indent = (firstNonWs == std::string::npos) ? "" : rawLine.substr(0, firstNonWs);
                            out << indent << "\"material\" \"tools/toolsnodraw\"\n";
                        }
                        else {
                            out << modLine << "\n";
                        }
                        continue;
                    }
                    else {
                        // Face visible : on recopie tel quel
                        out << rawLine << "\n";
                        continue;
                    }
                }

                // Ligne quelconque à l’intérieur du side
                out << rawLine << "\n";
                continue;
            }

            // Fin d'un solid (quand on remonte à la profondeur 0)
            if (!inSide && solidBraceDepth == 0) {
                out << rawLine << "\n";
                inSolid = false;
                continue;
            }

            // Ligne quelconque à l’intérieur du solid (hors side)
            out << rawLine << "\n";
            continue;
        }

        // Ligne hors de tout solid
        out << rawLine << "\n";
    }

    in.close();
    out.close();

    std::cout << "Optimized VMF written to " << dstPath << "\n";
}
