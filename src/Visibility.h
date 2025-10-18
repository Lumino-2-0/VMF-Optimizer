#pragma once
#include "Geometry.h"
#include <vector>

class Visibility {
public:
    // Detect hidden faces and return pointers to hidden Face objects.
    // The function also marks face.hidden = true for detected faces.
    static std::vector<Face*> DetectHiddenFaces(std::vector<Brush>& brushes);
};
