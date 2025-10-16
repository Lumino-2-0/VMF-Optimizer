#include "Visibility.h"
#include <iostream>

// Vérifie si deux AABB se chevauchent
bool Overlaps(const Brush& a, const Brush& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
        (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
        (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// Détecte les faces non visibles
void Visibility::DetectHiddenFaces(std::vector<Brush>& brushes) {
    int hiddenCount = 0;

    for (auto& brush : brushes) {
        for (auto& face : brush.faces) {
            bool hidden = false;

            // Comparer avec tous les autres brushes
            for (auto& other : brushes) {
                if (other.id == brush.id) continue; // pas se comparer avec soi-même
                if (!Overlaps(brush, other)) continue; // pas de chevauchement → skip

                // Approximation : si le centre de la face est dans l’AABB d’un autre brush → face cachée
                if (face.center.x >= other.min.x && face.center.x <= other.max.x &&
                    face.center.y >= other.min.y && face.center.y <= other.max.y &&
                    face.center.z >= other.min.z && face.center.z <= other.max.z) {
                    hidden = true;
                    break;
                }
            }

            face.hidden = hidden;
            if (hidden) hiddenCount++;
        }
    }

    std::cout << "Detected " << hiddenCount << " hidden faces." << std::endl;
}
