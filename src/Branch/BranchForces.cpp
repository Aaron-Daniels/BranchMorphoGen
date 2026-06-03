/******************************************************************************
 * File:        Simulation.cpp
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module calculates the bending and stretching forces
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License (or specify another if applicable).
 *
 * Last Modified:    Dec 02, 2025
 * This version is memory heavy but unconditionally stable for large time steps
 * For less memory heavy version with conditional stability use cgsolve method
 * Usage Notes:
 *   All units are in microns and seconds unless otherwise specified.
 *   Requires C++17 or later.
 *   Uses the Eigen library for sparse matrix operations.
 *   Dependencies: This codebase needs CMake, libtiff
 *
 * References:
 *   [1] Shree, Sutradhar, et al., Sci. Adv. 8, eabn0080 (2022) 29 June 2022
 *   [2] Ouyang, Sutradhar et al.  Nature Comm.
 *
 ******************************************************************************/

#include <algorithm> // for std::min
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <vector>

#include "Branch/BranchForces.h"

///////////////////////// include Eigen with warnings disabled /////////////////////////
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <Eigen/Sparse>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

//////////////////////////////////////////////////////////////////////////////
static inline double Area(double radius)
{
    return M_PI * radius * radius;
}

static inline double Moment_Inertia(double radius)
{
    return 0.25 * M_PI * radius * radius * radius * radius;
}


/*
static inline double Q_rsqrt(double number) noexcept
{
    double x2 = number * 0.5;
    double y  = number;
    uint64_t i;
    std::memcpy(&i, &y, sizeof(double));
    i  = 0x5fe6ec85e7de30daULL - (i >> 1);
    std::memcpy(&y, &i, sizeof(uint64_t));
    y = y * (1.5 - (x2 * y * y));      // one Newton step
    //y = y * (1.5 - (x2 * y * y));   // optional second step
    return y;
}
static inline Vec3 calculateStretchingForce(const Vec3& xi,
                                            const Vec3& xj,
                                            double rest_length,
                                            double K) noexcept
{
    // Early return if the spring is off
    if (K == 0.0) return {0.0, 0.0, 0.0};

    const Vec3 d = xj - xi;
    const double dd = d.squaredNorm();          // dot(d, d)

    if (dd < EPSILON) return {0.0, 0.0, 0.0};

    // K * (dis - rest) / dis == K * (1 - rest * inv_dis)
    Vec3 f = d * (K * (1.0 - rest_length * Q_rsqrt(dd)));
    //return d * (K * (1.0 - rest_length * std::sqrt(1.0/dd)));
    return f;

}

*/

static inline Vec3 calculateStretchingForce(
    const Vec3& xi,
    const Vec3& xj,
    double rest_length,
    double K
) noexcept
{
    if (K == 0.0) return {0.0, 0.0, 0.0};

    const Vec3 d = xj - xi;
    const double dd = d.squaredNorm();

    if (dd < EPSILON) return {0.0, 0.0, 0.0};

    const double dist = std::sqrt(dd);

    // raw extension
    const double max_extension = 0.9*rest_length;
    const double extension = dist - rest_length;

    // smooth saturation of extension
    const double eff_extension =
        max_extension * std::tanh(extension / max_extension);

    // force direction preserved
    return d * (K * eff_extension / dist);
}




/*
static inline Vec3 calculateBendingForce(const Vec3 &ileft,
                                         const Vec3 &imiddle,
                                         const Vec3 &iright,
                                         double K)
{
    Vec3 dx1 = imiddle - ileft;
    Vec3 dx2 = iright - imiddle;
    //return (iright + ileft - imiddle * 2.0) * K;
    return (dx2 - dx1) * K;
}
*/

namespace
{

using SparseMat = Eigen::SparseMatrix<double>;
using Triplet   = Eigen::Triplet<double>;

// Map (branch, local point) to global point index
struct GlobalIndexMap {
    std::vector<std::size_t> branchOffset; // starting point index of each branch
    std::size_t totalPoints = 0;
};

GlobalIndexMap buildGlobalIndex(const std::vector<Branch>& allBranches)
{
    GlobalIndexMap map;
    map.branchOffset.resize(allBranches.size());
    std::size_t offset = 0;
    for (std::size_t i = 0; i < allBranches.size(); ++i) {
        map.branchOffset[i] = offset;
        offset += allBranches[i].Points.size();
    }
    map.totalPoints = offset;
    return map;
}

inline std::size_t pointGlobalIndex(const GlobalIndexMap& map,
                                    std::size_t branchIdx,
                                    std::size_t localIdx)
{
    return map.branchOffset[branchIdx] + localIdx;
}

// Record of ground springs for root anchoring
struct RootSpring {
    std::size_t gid;   // global index of anchored node
    double k_eff;      // effective stiffness muDt * ks_root
};

// Add bending block for triplet (left, middle, right) with stiffness k_eff = mu * Dt * kb
void addBendTriplet(std::vector<Triplet>& trips,
                    std::size_t gidL,
                    std::size_t gidM,
                    std::size_t gidR,
                    double k_eff)
{
    if (k_eff == 0.0) return;

    // coefficients of L^T L for L = [1, -2, 1]
    static const double c[3][3] = {
        {  1.0, -2.0,  1.0 },
        { -2.0,  4.0, -2.0 },
        {  1.0, -2.0,  1.0 }
    };

    const std::size_t g[3] = { gidL, gidM, gidR };

    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 3; ++b) {
            const double val = k_eff * c[a][b];
            if (val == 0.0) continue;
            for (int d = 0; d < 3; ++d) {
                const int ia = static_cast<int>(3 * g[a] + d);
                const int ib = static_cast<int>(3 * g[b] + d);
                trips.emplace_back(ia, ib, val);
            }
        }
    }
}

// Ground spring: node gid attached to a fixed point with stiffness k_eff
// Contribution to A: for each component i,  A(i,i) += k_eff
void addGroundSpring(std::vector<Triplet>& trips,
                     std::size_t gid,
                     double k_eff)
{
    if (k_eff == 0.0) return;

    for (int d = 0; d < 3; ++d) {
        const int i = static_cast<int>(3 * gid + d);
        trips.emplace_back(i, i, k_eff);
    }
}

} // anonymous namespace

void BranchForces::updatePosition_ImplicitGlobal(std::vector<Branch>& allBranches,
                                                 const SimulationParameters& params,
                                                 const double DT_multiplier)
{
    const double ds_eff = params.BranchInterval;
    const double Dt     = params.Dt * DT_multiplier;
    const double mu     = params.DragInv;

    const std::size_t Nbranches = allBranches.size();
    if (Nbranches == 0) return;

    GlobalIndexMap gmap = buildGlobalIndex(allBranches);
    const std::size_t totalPoints = gmap.totalPoints;
    if (totalPoints == 0) return;

    const std::size_t ndof = 3 * totalPoints;
    const double ds3       = ds_eff * ds_eff * ds_eff;
    const double muDt      = mu * Dt;

    // Pack positions at start of step
    Eigen::VectorXd x_old(ndof);
    for (std::size_t bi = 0; bi < Nbranches; ++bi) {
        const auto& pts = allBranches[bi].Points;
        for (std::size_t j = 0; j < pts.size(); ++j) {
            const std::size_t gid = pointGlobalIndex(gmap, bi, j);
            x_old[3 * gid + 0] = pts[j].x;
            x_old[3 * gid + 1] = pts[j].y;
            x_old[3 * gid + 2] = pts[j].z;
        }
    }

    // 1) Assemble A = I + mu * Dt * K_bend (bending only) and ground springs
    std::vector<Triplet> trips;
    trips.reserve(static_cast<std::size_t>(ndof) * 10);

    // Root spring records for correct anchoring to params.Root
    std::vector<RootSpring> rootSprings;
    rootSprings.reserve(Nbranches);

    // Identity
    for (int i = 0; i < static_cast<int>(ndof); ++i) {
        trips.emplace_back(i, i, 1.0);
    }

    for (std::size_t bi = 0; bi < Nbranches; ++bi) {

        const auto& branch = allBranches[bi];
        const double radius = branch.Radius;
        const std::size_t np = branch.Points.size();
        if (np == 0) continue;

        const int pid = branch.Parent_ID;
        const int ch1 = branch.Child1_ID;
        const int ch2 = branch.Child2_ID;

        // Implicit ground spring for initial branches at their first point
        if (branch.Initial && np > 0) {
            const std::size_t gid0 = pointGlobalIndex(gmap, bi, 0);

            const double ks_root   = params.E_Axial * Area(radius) / ds_eff;
            const double k_eff_root = muDt * ks_root;

            addGroundSpring(trips, gid0, k_eff_root);

            // Store to add constant term for anchor at params.Root
            rootSprings.push_back(RootSpring{gid0, k_eff_root});
        }

        // Bending with parent at branch base (j = 0)
        // xl = parent tip, xm = P[0], xr = P[1]
        if (np >= 2 && pid >= 0 && pid < static_cast<int>(Nbranches) && !branch.Initial) {
            const auto& brP = allBranches[pid];
            const std::size_t npP = brP.Points.size();
            if (npP > 0) {
                const std::size_t gidL = pointGlobalIndex(
                    gmap, static_cast<std::size_t>(pid), npP - 1); // parent tip
                const std::size_t gidM = pointGlobalIndex(gmap, bi, 0); // child P[0]
                const std::size_t gidR = pointGlobalIndex(gmap, bi, 1); // child P[1]

                const double rav   = 0.5 * (radius + brP.Radius);
                const double kb_pb = params.E_Bending * Moment_Inertia(rav) / ds3;
                const double k_eff = muDt * kb_pb;

                addBendTriplet(trips, gidL, gidM, gidR, k_eff);
            }
        }
        // pid < 0 root branch bending against fixed root would add a constant term
        // and does not appear in K_bend, so that part is kept out of A.

        // Interior bending for this branch
        if (np >= 3) {
            const double kb_seg     = params.E_Bending * Moment_Inertia(radius) / ds3;
            const double k_eff_bend = muDt * kb_seg;

            for (std::size_t j = 1; j + 1 < np; ++j) {
                const std::size_t gidL = pointGlobalIndex(gmap, bi, j - 1);
                const std::size_t gidM = pointGlobalIndex(gmap, bi, j);
                const std::size_t gidR = pointGlobalIndex(gmap, bi, j + 1);
                addBendTriplet(trips, gidL, gidM, gidR, k_eff_bend);
            }
        }

        if (np > 1) {
            const std::size_t gidTip  = pointGlobalIndex(gmap, bi, np - 1);
            const std::size_t gidPrev = pointGlobalIndex(gmap, bi, np - 2);

            // Parent child 1 bending: xl = P[j-1], xm = P[j], xr = child1[0]
            if (ch1 >= 0 && ch1 < static_cast<int>(Nbranches)) {
                const auto& brC1 = allBranches[ch1];
                if (!brC1.Points.empty()) {
                    const std::size_t gidC1 = pointGlobalIndex(
                        gmap, static_cast<std::size_t>(ch1), 0);
                    const double rav   = 0.5 * (radius + brC1.Radius);
                    const double kb_pc = params.E_Bending * Moment_Inertia(rav) / ds3;
                    addBendTriplet(trips, gidPrev, gidTip, gidC1, muDt * kb_pc);
                }
            }

            // Parent child 2 bending
            if (ch2 >= 0 && ch2 < static_cast<int>(Nbranches)) {
                const auto& brC2 = allBranches[ch2];
                if (!brC2.Points.empty()) {
                    const std::size_t gidC2 = pointGlobalIndex(
                        gmap, static_cast<std::size_t>(ch2), 0);
                    const double rav   = 0.5 * (radius + brC2.Radius);
                    const double kb_pc = params.E_Bending * Moment_Inertia(rav) / ds3;
                    addBendTriplet(trips, gidPrev, gidTip, gidC2, muDt * kb_pc);
                }
            }

            // Child 1 child 2 bending: xl = child2[0], xm = tip, xr = child1[0]
            if (ch1 >= 0 && ch2 >= 0 &&
                ch1 < static_cast<int>(Nbranches) &&
                ch2 < static_cast<int>(Nbranches)) {

                const auto& brC1 = allBranches[ch1];
                const auto& brC2 = allBranches[ch2];

                if (!brC1.Points.empty() && !brC2.Points.empty()) {
                    const std::size_t gidC1 = pointGlobalIndex(
                        gmap, static_cast<std::size_t>(ch1), 0);
                    const std::size_t gidC2 = pointGlobalIndex(
                        gmap, static_cast<std::size_t>(ch2), 0);

                    const double rav   = 0.5 * (brC1.Radius + brC2.Radius);
                    const double kb_cc = params.E_Bending * Moment_Inertia(rav) / ds3;

                    addBendTriplet(trips, gidC2, gidTip, gidC1, muDt * kb_cc);
                }
            }
        }
    }

    SparseMat A(static_cast<int>(ndof), static_cast<int>(ndof));
    A.setFromTriplets(trips.begin(), trips.end());

    Eigen::SimplicialLDLT<SparseMat> solver;
    solver.compute(A);
    if (solver.info() != Eigen::Success) {
        std::cerr << "updatePosition_ImplicitGlobal: factorization failed\n";
        return;
    }

    // 2) Build stretching force F_stretch(x_old) with correct rest lengths
    Eigen::VectorXd f_stretch(ndof);
    f_stretch.setZero();

    for (std::size_t bi = 0; bi < Nbranches; ++bi) {

        const auto& branch = allBranches[bi];
        const double radius = branch.Radius;
        const std::size_t np = branch.Points.size();
        if (np == 0) continue;

        const auto& P = branch.Points;

        // Axial stiffness for this branch
        const double ks_seg = params.E_Axial * Area(radius) / ds_eff;

        // Interior segments, with last rest length = LastDist
        for (std::size_t j = 0; j + 1 < np; ++j) {
            const std::size_t gidA = pointGlobalIndex(gmap, bi, j);
            const std::size_t gidB = pointGlobalIndex(gmap, bi, j + 1);

            const Vec3& xi = P[j];
            const Vec3& xj = P[j + 1];

            const double l0 = (j == np - 2) ? branch.LastDist : ds_eff;

            Vec3 fs = calculateStretchingForce(xi, xj, l0, ks_seg);

            f_stretch[3 * gidA + 0] += fs.x;
            f_stretch[3 * gidA + 1] += fs.y;
            f_stretch[3 * gidA + 2] += fs.z;

            f_stretch[3 * gidB + 0] -= fs.x;
            f_stretch[3 * gidB + 1] -= fs.y;
            f_stretch[3 * gidB + 2] -= fs.z;
        }

        // Tip to children stretching
        if (np > 0) {
            const int ch1 = branch.Child1_ID;
            const int ch2 = branch.Child2_ID;

            const std::size_t gidTip = pointGlobalIndex(gmap, bi, np - 1);
            const Vec3& xm = P[np - 1];

            if (ch1 >= 0 && ch1 < static_cast<int>(Nbranches)) {
                const auto& brC1 = allBranches[ch1];
                if (!brC1.Points.empty()) {
                    const std::size_t gidC1 = pointGlobalIndex(
                        gmap, static_cast<std::size_t>(ch1), 0);
                    const Vec3& xr = brC1.Points[0];
                    const double rav   = 0.5 * (radius + brC1.Radius);
                    const double ks_pc = params.E_Axial * Area(rav) / ds_eff;
                    const double l0    = ds_eff;

                    Vec3 fs = calculateStretchingForce(xm, xr, l0, ks_pc);

                    f_stretch[3 * gidTip + 0] += fs.x;
                    f_stretch[3 * gidTip + 1] += fs.y;
                    f_stretch[3 * gidTip + 2] += fs.z;

                    f_stretch[3 * gidC1 + 0] -= fs.x;
                    f_stretch[3 * gidC1 + 1] -= fs.y;
                    f_stretch[3 * gidC1 + 2] -= fs.z;
                }
            }

            if (ch2 >= 0 && ch2 < static_cast<int>(Nbranches)) {
                const auto& brC2 = allBranches[ch2];
                if (!brC2.Points.empty()) {
                    const std::size_t gidC2 = pointGlobalIndex(
                        gmap, static_cast<std::size_t>(ch2), 0);
                    const Vec3& xr = brC2.Points[0];
                    const double rav   = 0.5 * (radius + brC2.Radius);
                    const double ks_pc = params.E_Axial * Area(rav) / ds_eff;
                    const double l0    = ds_eff;

                    Vec3 fs = calculateStretchingForce(xm, xr, l0, ks_pc);

                    f_stretch[3 * gidTip + 0] += fs.x;
                    f_stretch[3 * gidTip + 1] += fs.y;
                    f_stretch[3 * gidTip + 2] += fs.z;

                    f_stretch[3 * gidC2 + 0] -= fs.x;
                    f_stretch[3 * gidC2 + 1] -= fs.y;
                    f_stretch[3 * gidC2 + 2] -= fs.z;
                }
            }
        }
    }

    // 3) Solve (I + mu * Dt * K_bend) x_new = x_old + mu * Dt * F_stretch(x_old) + ground terms
    Eigen::VectorXd rhs = x_old + muDt * f_stretch;

    // Ground springs anchored at params.Root, not at the origin
    for (const auto& rs : rootSprings) {
        const std::size_t gid = rs.gid;
        const double k_eff    = rs.k_eff;

        rhs[3 * gid + 0] += k_eff * params.Root.x;
        rhs[3 * gid + 1] += k_eff * params.Root.y;
        rhs[3 * gid + 2] += k_eff * params.Root.z;
    }

    Eigen::VectorXd x_new = solver.solve(rhs);
    if (solver.info() != Eigen::Success) {
        std::cerr << "updatePosition_ImplicitGlobal: solve failed\n";
        return;
    }

    // Scatter back and update child bases
    for (std::size_t bi = 0; bi < Nbranches; ++bi) {
        auto& br  = allBranches[bi];
        auto& pts = br.Points;
        const std::size_t np = pts.size();
        if (np == 0) continue;

        for (std::size_t j = 0; j < np; ++j) {
            const std::size_t gid = pointGlobalIndex(gmap, bi, j);
            pts[j].x = x_new[3 * gid + 0];
            pts[j].y = x_new[3 * gid + 1];
            pts[j].z = x_new[3 * gid + 2];
        }

        if (!br.Dynamic && np > 0) {
            const Vec3 base = pts.back();
            const int c1 = br.Child1_ID;
            const int c2 = br.Child2_ID;

            if (c1 >= 0 && c1 < static_cast<int>(Nbranches)) {
                allBranches[c1].BasePoint = base;
            }
            if (c2 >= 0 && c2 < static_cast<int>(Nbranches)) {
                allBranches[c2].BasePoint = base;
            }
            if (br.Initial) {
                br.BasePoint = params.Root;
            }
        }

        br.LastDist = br.calculateLastDist();
        br.Length   = br.calculateContourLength();
        br.updateBoundingBox();
    }
}
