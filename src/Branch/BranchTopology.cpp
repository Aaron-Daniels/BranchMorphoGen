/******************************************************************************
 * File:        BranchTopology.cpp
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module calculates branch topology to calculate radius
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License (or specify another if applicable).
 *
 * Last Modified:    Jul 1, 2025
 *
 * Usage Notes:
 *   - All units are in microns and seconds unless otherwise specified.
 *   - Requires C++17 or later.
 *   - Dependencies: This codebase needs CMake, libtiff
 *
 * References:
 *   [1] Shree, Sutradhar, et al., Sci. Adv. 8, eabn0080 (2022) 29 June 2022
 *   [2] Ouyang, Sutradhar et al.  Nature Comm. 
 *
 ******************************************************************************/
#include <iostream>
#include <cmath>
#include <stack>
#include "Utilities.h"
#include "Branch/BranchCommon.h"
#include "Branch/BranchTopology.h"

    // Recursively compute subtree statistics for branch i.
    // This matches the original SubtreeSearch logic.
void BranchTopology::computeSubtreeStats(
        size_t i,
        const std::vector<Branch> &allBranches,
        std::vector<char> &visited,
        std::vector<size_t> &leafno,
        std::vector<size_t> &branchno,
        std::vector<double> &subtree_length,
        std::vector<double> &subtree_area,
        std::vector<double> &subtree_volume)
    {
        if (visited[i])
            return;

        visited[i] = 1;

        const Branch &branch = allBranches[i];

        // own contribution
        branchno[i] = 1;
        subtree_length[i] = branch.Length;
        subtree_area[i] = M_PI * branch.Radius * branch.Length;
        subtree_volume[i] = M_PI * branch.Radius * branch.Radius * branch.Length;

        // decide whether to descend into children, same condition as SubtreeSearch
        if (!branch.Dynamic && branch.Child1_ID >= 0 && branch.Child2_ID >= 0)
        {
            const size_t c1 = static_cast<size_t>(branch.Child1_ID);
            const size_t c2 = static_cast<size_t>(branch.Child2_ID);

            // ensure children are computed first
            computeSubtreeStats(c1, allBranches, visited,
                                leafno, branchno,
                                subtree_length, subtree_area, subtree_volume);
            computeSubtreeStats(c2, allBranches, visited,
                                leafno, branchno,
                                subtree_length, subtree_area, subtree_volume);

            // combine children
            branchno[i] += branchno[c1] + branchno[c2];
            subtree_length[i] += subtree_length[c1] + subtree_length[c2];
            subtree_area[i] += subtree_area[c1] + subtree_area[c2];
            subtree_volume[i] += subtree_volume[c1] + subtree_volume[c2];
            leafno[i] = leafno[c1] + leafno[c2];
        }
        else
        {
            // terminal node for this definition
            leafno[i] = 1;
        }
}

void BranchTopology::updateRadius(std::vector<Branch> &allBranches,
                                  const SimulationParameters &params)
{
    const size_t n = allBranches.size();
    if (n == 0)
        return;

    // storage for subtree data of every branch
    std::vector<char> visited(n, 0);
    std::vector<size_t> leafno(n, 0);
    std::vector<size_t> branchno(n, 0);
    std::vector<double> subtree_length(n, 0.0);
    std::vector<double> subtree_area(n, 0.0);
    std::vector<double> subtree_volume(n, 0.0);

    // compute subtree stats for every branch, visiting each branch and edge once
    for (size_t i = 0; i < n; ++i)
    {
        if (!visited[i])
        {
            computeSubtreeStats(i, allBranches,
                                visited, leafno, branchno,
                                subtree_length, subtree_area, subtree_volume);
        }
    }

    const double rmin2 = params.MinimumRadius * params.MinimumRadius;
    const double beta = params.BetaRadius;

    for (size_t i = 0; i < n; ++i)
    {
        double quantity = 0.0;

        if (params.RadiusModel == "none")
        {
            quantity = params.MinimumRadius;
        }
        else if (params.RadiusModel == "length")
        {
            quantity = subtree_length[i];
        }
        else if (params.RadiusModel == "area")
        {
            quantity = subtree_area[i];
        }
        else if (params.RadiusModel == "volume")
        {
            quantity = subtree_volume[i];
        }
        else if (params.RadiusModel == "tipnumber")
        {
            quantity = static_cast<double>(leafno[i]);
        }
        else if (params.RadiusModel == "branchnumber")
        {
            quantity = static_cast<double>(branchno[i]);
        }
        else
        {
            throw std::invalid_argument("Unknown RadiusModel: " + params.RadiusModel);
        }

        allBranches[i].Radius =
            std::min(std::sqrt(rmin2 + beta * quantity / M_PI),
                     params.MaximumRadius);
    }
}

 // namespace BranchTopology


