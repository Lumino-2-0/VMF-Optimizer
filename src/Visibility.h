#pragma once
#include <vector>
#include "VMFParser.h"   // On utilise les structs déjà définis ici

namespace Visibility {
    // Détecte les faces cachées dans un ensemble de brushes
    void DetectHiddenFaces(std::vector<Brush>& brushes);
}
