#include "Visibility.h"
#include <algorithm>
#include <cmath>
#include <iostream>

void Visibility::DetectHiddenFaces(std::vector<Brush>& brushes)
{
    const double NORMAL_DOT_MIN = -0.85;
    const double PLANE_EPS = 0.5;
    const double COVER_RATIO = 0.75;

    auto getFacePoints = [](const Face& f) -> std::vector<Vec3>
        {
            if (!f.vertices.empty())
                return f.vertices;

            return { f.p1, f.p2, f.p3 };
        };

    auto buildBasis = [](const Vec3& n, Vec3& u, Vec3& v) -> bool
        {
            Vec3 ref = (std::fabs(n.z) < 0.9) ? Vec3{ 0, 0, 1 } : Vec3{ 0, 1, 0 };
            u = Normalize(Cross(n, ref));
            if (Length(u) < 1e-6)
                return false;

            v = Normalize(Cross(n, u));
            return Length(v) >= 1e-6;
        };

    auto project = [](const std::vector<Vec3>& pts, const Vec3& u, const Vec3& v,
        double& minU, double& maxU, double& minV, double& maxV)
        {
            minU = maxU = Dot(pts[0], u);
            minV = maxV = Dot(pts[0], v);

            for (size_t i = 1; i < pts.size(); ++i)
            {
                double pu = Dot(pts[i], u);
                double pv = Dot(pts[i], v);

                if (pu < minU) minU = pu;
                if (pu > maxU) maxU = pu;
                if (pv < minV) minV = pv;
                if (pv > maxV) maxV = pv;
            }
        };

    for (auto& b : brushes)
        for (auto& f : b.faces)
            f.hidden = false;

    int hiddenCount = 0;

    for (auto& A : brushes)
    {
        for (auto& fA : A.faces)
        {
            std::vector<Vec3> ptsA = getFacePoints(fA);
            if (ptsA.size() < 3)
                continue;

            Vec3 nA = Normalize(fA.normal);
            if (Length(nA) < 1e-6)
                continue;

            Vec3 u, v;
            if (!buildBasis(nA, u, v))
                continue;

            double aMinU, aMaxU, aMinV, aMaxV;
            project(ptsA, u, v, aMinU, aMaxU, aMinV, aMaxV);

            double areaA = (aMaxU - aMinU) * (aMaxV - aMinV);
            if (areaA <= 1e-6)
                continue;

            double planeA = Dot(nA, ptsA[0]);

            for (auto& B : brushes)
            {
                if (A.id == B.id)
                    continue;

                for (auto& fB : B.faces)
                {
                    std::vector<Vec3> ptsB = getFacePoints(fB);
                    if (ptsB.size() < 3)
                        continue;

                    Vec3 nB = Normalize(fB.normal);
                    if (Length(nB) < 1e-6)
                        continue;

                    if (Dot(nA, nB) > NORMAL_DOT_MIN)
                        continue;

                    double planeB = Dot(nA, ptsB[0]);
                    if (std::fabs(planeB - planeA) > PLANE_EPS)
                        continue;

                    double bMinU, bMaxU, bMinV, bMaxV;
                    project(ptsB, u, v, bMinU, bMaxU, bMinV, bMaxV);

                    double overlapU = std::max(0.0, std::min(aMaxU, bMaxU) - std::max(aMinU, bMinU));
                    double overlapV = std::max(0.0, std::min(aMaxV, bMaxV) - std::max(aMinV, bMinV));

                    if (overlapU <= 0.0 || overlapV <= 0.0)
                        continue;

                    double coverage = (overlapU * overlapV) / areaA;
                    if (coverage >= COVER_RATIO)
                    {
                        fA.hidden = true;
                        hiddenCount++;
                        goto nextFace;
                    }
                }
            }

        nextFace:;
        }
    }

    std::cout << "Detected " << hiddenCount << " hidden faces.\n";
}
