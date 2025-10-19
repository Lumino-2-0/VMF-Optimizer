#include "Visibility.h"
#include <cmath>
#include <algorithm>
#include <iostream>

static inline double Abs(double v) { return v < 0 ? -v : v; }
static inline double Clamp(double v, double a, double b) { return std::max(a, std::min(b, v)); }

// Petites tolérances pour compenser les flottants/arrondis Hammer
static constexpr double EPS_PLANE = 0.25;   // tolérance coplanarité (unités Hammer)
static constexpr double EPS_OVERLAP = 0.25;   // tolérance recouvrement
static constexpr double EPS_AXIS = 1e-3;   // tolérance alignement composantes
static constexpr double AXIS_ALIGN_COS = 0.999; // normal quasi alignée avec un axe

enum Axis { AX = 0, AY = 1, AZ = 2 };

// renvoie l'axe dominant de la normale et le signe (+1 si vers +axis, -1 sinon)
// true si la face est quasi axis-aligned
static bool GetAxisAligned(const Vec3& n, Axis& axis, int& sign)
{
    double ax = Abs(n.x), ay = Abs(n.y), az = Abs(n.z);
    if (ax >= ay && ax >= az) {
        if (ax < AXIS_ALIGN_COS) return false;
        axis = AX; sign = (n.x >= 0.0) ? +1 : -1; return true;
    }
    else if (ay >= ax && ay >= az) {
        if (ay < AXIS_ALIGN_COS) return false;
        axis = AY; sign = (n.y >= 0.0) ? +1 : -1; return true;
    }
    else {
        if (az < AXIS_ALIGN_COS) return false;
        axis = AZ; sign = (n.z >= 0.0) ? +1 : -1; return true;
    }
}

// renvoie la coordonnée du plan pour l'axe donné (via le centre de la face)
static double PlaneCoordOnAxis(const Face& f, Axis a) {
    return (a == AX) ? f.center.x : (a == AY ? f.center.y : f.center.z);
}

// accès min/max d’un brush sur un axe
static double MinOn(const Brush& b, Axis a) { return (a == AX) ? b.min.x : (a == AY ? b.min.y : b.min.z); }
static double MaxOn(const Brush& b, Axis a) { return (a == AX) ? b.max.x : (a == AY ? b.max.y : b.max.z); }

// aire du rectangle 2D projeté
static inline double RectArea(double a0, double a1, double b0, double b1) {
    return std::max(0.0, a1 - a0) * std::max(0.0, b1 - b0);
}

// recouvrement de 2 intervalles [a0,a1] et [b0,b1]
static inline double OverlapLen(double a0, double a1, double b0, double b1) {
    double lo = std::max(a0, b0);
    double hi = std::min(a1, b1);
    return std::max(0.0, hi - lo);
}

// marque une face fA cachée si elle est entièrement couverte par le rectangle opposé de B (mêmes coordonnées de plan)
// axis : axe normal, sign : sens (+1 max, -1 min)
static void TestCoverFaceAgainstBrush(Face& fA, const Brush& A, const Brush& B, Axis axis, int sign)
{
    if (fA.hidden) return; // déjà cachée

    // le plan de la face sur l’axe "axis"
    const double planeCoord = PlaneCoordOnAxis(fA, axis);

    // pour que B soit "contre" cette face :
    //  - si la face regarde +axis (sign=+1), elle est sur Max(A,axis), on veut que Min(B,axis) ~ planeCoord
    //  - si sign=-1, elle est sur Min(A,axis), on veut que Max(B,axis) ~ planeCoord
    const double aMin = MinOn(A, axis), aMax = MaxOn(A, axis);
    const double bMin = MinOn(B, axis), bMax = MaxOn(B, axis);

    // vérifie que la face est effectivement sur un côté de A
    double aSideCoord = (sign > 0) ? aMax : aMin;
    if (Abs(planeCoord - aSideCoord) > EPS_PLANE) return;

    // vérifie que B touche ce plan
    if (sign > 0) {
        if (Abs(bMin - planeCoord) > EPS_PLANE) return; // B doit commencer exactement là
    }
    else {
        if (Abs(bMax - planeCoord) > EPS_PLANE) return; // B doit finir exactement là
    }

    // axes “dans le plan”
    Axis u = (axis == AX) ? AY : AX;
    Axis v = (axis == AZ) ? AY : AZ;
    if (axis == AY) { u = AX; v = AZ; }

    // rectangles projetés (A face vs. B côté opposé)
    double Au0 = MinOn(A, u), Au1 = MaxOn(A, u);
    double Av0 = MinOn(A, v), Av1 = MaxOn(A, v);
    double Bu0 = MinOn(B, u), Bu1 = MaxOn(B, u);
    double Bv0 = MinOn(B, v), Bv1 = MaxOn(B, v);

    // calcul recouvrements
    double ou = OverlapLen(Au0, Au1, Bu0, Bu1);
    double ov = OverlapLen(Av0, Av1, Bv0, Bv1);
    if (ou <= EPS_OVERLAP || ov <= EPS_OVERLAP) return; // pas de recouvrement utile

    double areaOverlap = ou * ov;
    double areaA = std::max(0.0, (Au1 - Au0)) * std::max(0.0, (Av1 - Av0));
    double areaB = std::max(0.0, (Bu1 - Bu0)) * std::max(0.0, (Bv1 - Bv0));

    if (areaA <= EPS_OVERLAP || areaB <= EPS_OVERLAP) return;

    // si la face A est complètement couverte par B au moins à 99%
    double covA = areaOverlap / areaA;
    if (covA >= 0.99) {
        // Règle :
        //  - si tailles égales (≈), on peut cacher les deux (lorsque l’autre face sera testée, elle se cachera aussi)
        //  - si A est plus petite (cas courant “petit bloc contre grand”), on cache A
        //  - si A est plus grande et B ne la recouvre pas entièrement, on ne cache pas A
        // => ici covA≈1 → on cache A (et l’autre côté sera traité quand ce sera son tour)
        fA.hidden = true;
    }
}

void Visibility::DetectHiddenFaces(std::vector<Brush>& brushes)
{
    int hiddenCount = 0;

    const double PLANE_EPS = 4.0;
    const double CENTER_EPS = 8.0;
    const double NORMAL_DOT = 0.80;   // jusqu’à 36°
    const double GAP_EPS = 4.0;

    for (Brush& A : brushes)
    {
        for (Face& fA : A.faces)
        {
            if (fA.hidden) continue;

            double lenA = std::sqrt(fA.normal.x * fA.normal.x + fA.normal.y * fA.normal.y + fA.normal.z * fA.normal.z);
            if (lenA < 0.001) continue;

            for (const Brush& B : brushes)
            {
                if (A.id == B.id) continue;

                // Direction du centre de B par rapport à A
                Vec3 dirAB{
                    B.min.x + (B.max.x - B.min.x) * 0.5 - fA.center.x,
                    B.min.y + (B.max.y - B.min.y) * 0.5 - fA.center.y,
                    B.min.z + (B.max.z - B.min.z) * 0.5 - fA.center.z
                };
                double lenDir = std::sqrt(dirAB.x * dirAB.x + dirAB.y * dirAB.y + dirAB.z * dirAB.z);
                if (lenDir < 0.001) continue;
                dirAB.x /= lenDir; dirAB.y /= lenDir; dirAB.z /= lenDir;

                // Si la face ne regarde PAS vers B, inutile
                double dirDot = fA.normal.x * dirAB.x + fA.normal.y * dirAB.y + fA.normal.z * dirAB.z;
                if (dirDot < 0.2) continue; // face regarde ailleurs → visible

                for (const Face& fB : B.faces)
                {
                    // Normales opposées
                    double dot = fA.normal.x * fB.normal.x + fA.normal.y * fB.normal.y + fA.normal.z * fB.normal.z;
                    if (dot > -NORMAL_DOT) continue;

                    // Distance des centres
                    double distCenter = std::sqrt(
                        std::pow(fA.center.x - fB.center.x, 2) +
                        std::pow(fA.center.y - fB.center.y, 2) +
                        std::pow(fA.center.z - fB.center.z, 2));

                    if (distCenter > CENTER_EPS)
                    {
                        Vec3 Amin = A.min, Amax = A.max;
                        Vec3 Bmin = B.min, Bmax = B.max;

                        double dx = std::max({ 0.0, Bmin.x - Amax.x, Amin.x - Bmax.x });
                        double dy = std::max({ 0.0, Bmin.y - Amax.y, Amin.y - Bmax.y });
                        double dz = std::max({ 0.0, Bmin.z - Amax.z, Amin.z - Bmax.z });

                        double gap = std::sqrt(dx * dx + dy * dy + dz * dz);
                        if (gap > GAP_EPS) continue;
                    }

                    // Vérifie recouvrement approximatif
                    Vec3 Amin = A.min, Amax = A.max;
                    Vec3 Bmin = B.min, Bmax = B.max;

                    bool overlapX = !(Amax.x < Bmin.x - PLANE_EPS || Amin.x > Bmax.x + PLANE_EPS);
                    bool overlapY = !(Amax.y < Bmin.y - PLANE_EPS || Amin.y > Bmax.y + PLANE_EPS);
                    bool overlapZ = !(Amax.z < Bmin.z - PLANE_EPS || Amin.z > Bmax.z + PLANE_EPS);
                    if (!(overlapX && overlapY && overlapZ)) continue;

                    // Si tout est bon → face masquée (vérifiée côté intérieur)
                    fA.hidden = true;
                    hiddenCount++;
                    break;
                }
                if (fA.hidden) break;
            }
        }
    }

    std::cout << "Detected " << hiddenCount << " hidden faces.\n";
}




 