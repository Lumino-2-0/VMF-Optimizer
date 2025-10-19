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
    // Tolerances adaptées au grid Hammer
    const double PLANE_EPS = 0.5;   // coplanarité sur l'axe normal (doit être serré)
    const double OVERLAP_EPS = 0.5;   // marge pour recouvrement sur les axes du plan
    const double COVER_RATIO = 0.98;  // % minimum de recouvrement de la face

    auto axisLen = [](const Brush& b, int axis)->double {
        if (axis == 0) return b.max.x - b.min.x;
        if (axis == 1) return b.max.y - b.min.y;
        return b.max.z - b.min.z;
        };
    auto minOn = [](const Brush& b, int axis)->double {
        return (axis == 0) ? b.min.x : (axis == 1) ? b.min.y : b.min.z;
        };
    auto maxOn = [](const Brush& b, int axis)->double {
        return (axis == 0) ? b.max.x : (axis == 1) ? b.max.y : b.max.z;
        };

    auto overlapLen = [&](double a0, double a1, double b0, double b1)->double {
        double lo = std::max(a0, b0);
        double hi = std::min(a1, b1);
        return std::max(0.0, hi - lo);
        };

    int hiddenCount = 0;

    for (Brush& A : brushes)
    {
        for (Face& fA : A.faces)
        {
            fA.hidden = false; // on repart propre
            // Détecte sur quel plan axis-aligned se trouve la face :
            // La face d’un brush axis-aligned est forcément sur l’un des 6 plans :
            // X = min.x | X = max.x | Y = min.y | Y = max.y | Z = min.z | Z = max.z
            // On n’utilise PAS la normale : on teste la position du centre par rapport aux plans du brush.

            // On cherche l’axe/side qui colle le mieux :
            int axis = -1;        // 0=X, 1=Y, 2=Z
            bool isMaxSide = false;

            // distance du centre à chacun des 6 plans
            struct Cand { int axis; bool isMax; double dist; };
            Cand best{ -1, false, 1e30 };

            // X min / max
            {
                double dMin = std::abs(fA.center.x - A.min.x);
                double dMax = std::abs(fA.center.x - A.max.x);
                if (dMin < best.dist) best = { 0, false, dMin };
                if (dMax < best.dist) best = { 0, true,  dMax };
            }
            // Y min / max
            {
                double dMin = std::abs(fA.center.y - A.min.y);
                double dMax = std::abs(fA.center.y - A.max.y);
                if (dMin < best.dist) best = { 1, false, dMin };
                if (dMax < best.dist) best = { 1, true,  dMax };
            }
            // Z min / max
            {
                double dMin = std::abs(fA.center.z - A.min.z);
                double dMax = std::abs(fA.center.z - A.max.z);
                if (dMin < best.dist) best = { 2, false, dMin };
                if (dMax < best.dist) best = { 2, true,  dMax };
            }

            // Si on n'est clairement pas sur un plan du brush → on ignore (face biseautée / non axis-aligned)
            if (best.dist > PLANE_EPS) {
                continue;
            }

            axis = best.axis;
            isMaxSide = best.isMax;

            // Dimensions de la face d’A sur les deux axes restants (u,v)
            int u = (axis == 0) ? 1 : 0;
            int v = (axis == 2) ? 1 : 2;
            if (axis == 1) { u = 0; v = 2; }

            double Au0 = minOn(A, u), Au1 = maxOn(A, u);
            double Av0 = minOn(A, v), Av1 = maxOn(A, v);
            double lenAu = std::max(0.0, Au1 - Au0);
            double lenAv = std::max(0.0, Av1 - Av0);
            if (lenAu <= OVERLAP_EPS || lenAv <= OVERLAP_EPS) {
                // face dégénérée ? on ne cache pas
                continue;
            }

            // Coordonnée du plan de la face d’A (sur 'axis')
            double Aplane = isMaxSide ? maxOn(A, axis) : minOn(A, axis);

            // Recherche d’un brush B qui touche exactement ce plan et recouvre (presque) toute la face d’A
            bool covered = false;

            for (const Brush& B : brushes)
            {
                if (B.id == A.id) continue;

                // B doit toucher A sur ce plan exact :
                double touch = isMaxSide ? (minOn(B, axis) - Aplane)   // B.min == A.max ?
                    : (maxOn(B, axis) - Aplane);  // B.max == A.min ?
                if (std::abs(touch) > PLANE_EPS) continue; // pas coplanaires (pas en contact réel)

                // Recouvrement projeté sur (u,v)
                double Bu0 = minOn(B, u), Bu1 = maxOn(B, u);
                double Bv0 = minOn(B, v), Bv1 = maxOn(B, v);

                double ou = overlapLen(Au0, Au1, Bu0, Bu1);
                double ov = overlapLen(Av0, Av1, Bv0, Bv1);

                // ratios de recouvrement par rapport à la face d’A
                double ru = (lenAu <= OVERLAP_EPS) ? 0.0 : (ou / lenAu);
                double rv = (lenAv <= OVERLAP_EPS) ? 0.0 : (ov / lenAv);

                if (ru >= COVER_RATIO && rv >= COVER_RATIO) {
                    covered = true;
                    break;
                }
            }

            if (covered) {
                fA.hidden = true;
                hiddenCount++;
            }
        }
    }

    std::cout << "Detected " << hiddenCount << " hidden faces.\n";
}





 