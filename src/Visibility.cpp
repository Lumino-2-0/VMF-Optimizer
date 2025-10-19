#include "Visibility.h"
#include <algorithm>
#include <cmath>
#include <iostream>

void Visibility::DetectHiddenFaces(std::vector<Brush>& brushes)
{
    const double NORMAL_EPS = 0.02;    // tolérance sur l'angle (~arccos(0.98) ≈ 11°)
    const double PLANE_EPS = 0.5;      // tolérance de coplanarité (unités Hammer)
    const double COVER_RATIO = 0.98;   // % minimum de recouvrement de la face testée

    auto buildBasis = [](const Vec3& normal, Vec3& u, Vec3& v) -> bool {
        Vec3 ref = { 0.0, 0.0, 1.0 };
        Vec3 tangent = Cross(normal, ref);
        if (Length(tangent) < 1e-4) {
            ref = { 0.0, 1.0, 0.0 };
            tangent = Cross(normal, ref);
        }
        double lenT = Length(tangent);
        if (lenT < 1e-4) return false;
        u = tangent * (1.0 / lenT);
        v = Normalize(Cross(normal, u));
        return Length(v) >= 1e-4;
    };

    auto projectExtents = [](const Face& face, const Vec3& u, const Vec3& v,
        double& uMin, double& uMax, double& vMin, double& vMax)
    {
        auto accumulate = [&](const Vec3& p) {
            double pu = Dot(p, u);
            double pv = Dot(p, v);
            if (pu < uMin) uMin = pu;
            if (pu > uMax) uMax = pu;
            if (pv < vMin) vMin = pv;
            if (pv > vMax) vMax = pv;
        };

        uMin = uMax = Dot(face.p1, u);
        vMin = vMax = Dot(face.p1, v);
        accumulate(face.p2);
        accumulate(face.p3);
        accumulate(face.center);
    };

    // On repart de zéro à chaque passe
    for (Brush& b : brushes) {
        for (Face& f : b.faces) {
            f.hidden = false;
        }
    }

    int hiddenCount = 0;

    for (Brush& A : brushes) {
        for (Face& fA : A.faces) {
            Vec3 nA = fA.normal;
            double lenA = Length(nA);
            if (lenA < 1e-4) continue;

            Vec3 u, v;
            if (!buildBasis(nA, u, v)) continue;

            double AuMin, AuMax, AvMin, AvMax;
            projectExtents(fA, u, v, AuMin, AuMax, AvMin, AvMax);
            double areaA = (AuMax - AuMin) * (AvMax - AvMin);
            if (areaA <= 1e-6) continue;

            double planeA = Dot(nA, fA.center);

            for (const Brush& B : brushes) {
                if (A.id == B.id) continue;

                for (const Face& fB : B.faces) {
                    Vec3 nB = fB.normal;
                    double lenB = Length(nB);
                    if (lenB < 1e-4) continue;

                    double normalDot = Dot(nA, nB);
                    if (normalDot > -(1.0 - NORMAL_EPS)) continue; // pas assez opposées

                    Vec3 delta = fB.center - fA.center;
                    double planeDelta = std::abs(Dot(delta, nA));
                    if (planeDelta > PLANE_EPS) continue; // pas sur le même plan

                    // B doit être "devant" la face (côté extérieur)
                    if (Dot(delta, nA) <= 0.0) continue;

                    // Vérifie la coplanarité via la projection sur le plan de A
                    double planePosB = Dot(nA, fB.center);
                    if (std::abs(planePosB - planeA) > PLANE_EPS) continue;

                    double BuMin, BuMax, BvMin, BvMax;
                    projectExtents(fB, u, v, BuMin, BuMax, BvMin, BvMax);

                    double overlapU = std::max(0.0, std::min(AuMax, BuMax) - std::max(AuMin, BuMin));
                    double overlapV = std::max(0.0, std::min(AvMax, BvMax) - std::max(AvMin, BvMin));
                    if (overlapU <= 0.0 || overlapV <= 0.0) continue;

                    double overlapArea = overlapU * overlapV;
                    double coverage = overlapArea / areaA;
                    if (coverage >= COVER_RATIO) {
                        fA.hidden = true;
                        hiddenCount++;
                        break;
                    }
                }

                if (fA.hidden) break;
            }
        }
    }

    std::cout << "Detected " << hiddenCount << " hidden faces.\n";
}




 
