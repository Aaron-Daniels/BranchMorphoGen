/******************************************************************************
 * File:        BranchForces.h
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
#pragma once
#include <iostream>
#include "SimulationParameters.h"
#include "Branch/BranchCommon.h"

namespace BranchTopology
{

void computeSubtreeStats(
        size_t i,
        const std::vector<Branch> &allBranches,
        std::vector<char> &visited,
        std::vector<size_t> &leafno,
        std::vector<size_t> &branchno,
        std::vector<double> &subtree_length,
        std::vector<double> &subtree_area,
        std::vector<double> &subtree_volume);

void updateRadius(std::vector<Branch> &allBranches,
                       const SimulationParameters &params);

    
}
/*

void SubtreeSearch(const std::vector<Branch> &allBranches, size_t i,
                            size_t &leafno, size_t& branchno, double &subtree_length,
                            double &subtree_area, double &subtree_volume);
*/