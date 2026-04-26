#include "Visibility.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void Visibility::DetectHiddenFaces(std::vector<Brush>& brushes)
{
    const double NORMAL_DOT_MIN = -0.85;
    const double PLANE_EPS = 0.5;

    // Une face est cachée seulement si sa surface est quasi totalement recouverte.
    // Cela évite de nodraw une face qui garde un liseré visible (cas V2).
    const double HIDDEN_COVER_RATIO = 0.98;

    // Ignore les micro-recouvrements parasites.
    const double MIN_PATCH_RATIO = 0.01;

    struct Rect2D
    {
        double minU;
        double maxU;
        double minV;
        double maxV;
    };

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
                const double pu = Dot(pts[i], u);
                const double pv = Dot(pts[i], v);

                if (pu < minU) minU = pu;
                if (pu > maxU) maxU = pu;
                if (pv < minV) minV = pv;
                if (pv > maxV) maxV = pv;
            }
        };

    auto computeUnionArea = [](const std::vector<Rect2D>& rects) -> double
        {
            if (rects.empty())
                return 0.0;

            std::vector<double> vCuts;
            vCuts.reserve(rects.size() * 2);

            for (const Rect2D& r : rects)
            {
                vCuts.push_back(r.minV);
                vCuts.push_back(r.maxV);
            }

            std::sort(vCuts.begin(), vCuts.end());
            vCuts.erase(std::unique(vCuts.begin(), vCuts.end()), vCuts.end());

            double area = 0.0;

            for (size_t i = 0; i + 1 < vCuts.size(); ++i)
            {
                const double v0 = vCuts[i];
                const double v1 = vCuts[i + 1];
                const double slabHeight = v1 - v0;

                if (slabHeight <= 1e-9)
                    continue;

                std::vector<std::pair<double, double>> intervals;
                intervals.reserve(rects.size());

                for (const Rect2D& r : rects)
                {
                    // Le rectangle participe à cette tranche en V.
                    if (r.maxV <= v0 || r.minV >= v1)
                        continue;

                    intervals.emplace_back(r.minU, r.maxU);
                }

                if (intervals.empty())
                    continue;

                std::sort(intervals.begin(), intervals.end());

                double mergedLen = 0.0;
                double curL = intervals[0].first;
                double curR = intervals[0].second;

                for (size_t k = 1; k < intervals.size(); ++k)
                {
                    const double L = intervals[k].first;
                    const double R = intervals[k].second;

                    if (L > curR)
                    {
                        mergedLen += (curR - curL);
                        curL = L;
                        curR = R;
                    }
                    else
                    {
                        if (R > curR)
                            curR = R;
                    }
                }

                mergedLen += (curR - curL);
                area += mergedLen * slabHeight;
            }

            return area;
        };

    for (auto& b : brushes)
        for (auto& f : b.faces)
            f.hidden = false;

    int hiddenCount = 0;

    for (auto& A : brushes)
    {
        for (auto& fA : A.faces)
        {
            const std::vector<Vec3> ptsA = getFacePoints(fA);
            if (ptsA.size() < 3)
                continue;

            const Vec3 nA = Normalize(fA.normal);
            if (Length(nA) < 1e-6)
                continue;

            Vec3 u, v;
            if (!buildBasis(nA, u, v))
                continue;

            double aMinU, aMaxU, aMinV, aMaxV;
            project(ptsA, u, v, aMinU, aMaxU, aMinV, aMaxV);

            const double areaA = (aMaxU - aMinU) * (aMaxV - aMinV);
            if (areaA <= 1e-6)
                continue;

            const double planeA = Dot(nA, ptsA[0]);
            std::vector<Rect2D> coverRects;
            coverRects.reserve(16);

            for (auto& B : brushes)
            {
                if (A.id == B.id)
                    continue;

                for (auto& fB : B.faces)
                {
                    const std::vector<Vec3> ptsB = getFacePoints(fB);
                    if (ptsB.size() < 3)
                        continue;

                    const Vec3 nB = Normalize(fB.normal);
                    if (Length(nB) < 1e-6)
                        continue;

                    // Face opposée (dos à dos) : candidat de masquage.
                    if (Dot(nA, nB) > NORMAL_DOT_MIN)
                        continue;

                    const double planeB = Dot(nA, ptsB[0]);
                    if (std::fabs(planeB - planeA) > PLANE_EPS)
                        continue;

                    double bMinU, bMaxU, bMinV, bMaxV;
                    project(ptsB, u, v, bMinU, bMaxU, bMinV, bMaxV);

                    const double oMinU = std::max(aMinU, bMinU);
                    const double oMaxU = std::min(aMaxU, bMaxU);
                    const double oMinV = std::max(aMinV, bMinV);
                    const double oMaxV = std::min(aMaxV, bMaxV);

                    if (oMaxU <= oMinU || oMaxV <= oMinV)
                        continue;

                    const double patchRatio = ((oMaxU - oMinU) * (oMaxV - oMinV)) / areaA;
                    if (patchRatio < MIN_PATCH_RATIO)
                        continue;

                    coverRects.push_back({ oMinU, oMaxU, oMinV, oMaxV });
                }
            }

            const double coveredArea = computeUnionArea(coverRects);
            const double coverage = coveredArea / areaA;

            // Important : recouvrement cumulé (union) et non recouvrement d'une seule face.
            // => V3 peut être cachée par V1+V2, tout en gardant V2 visible si un bord reste exposé.
            if (coverage >= HIDDEN_COVER_RATIO)
            {
                fA.hidden = true;
                hiddenCount++;
            }
        }
    }

    std::cout << "Detected " << hiddenCount << " hidden faces.\n";
}
