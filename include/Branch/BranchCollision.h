/******************************************************************************
 * File:        BranchCollision.cpp
 * Project:     Branching Morphogenesis Generator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *  This module calculates the intersections and crossings of ngighboring branches using a bounding box algorithm 
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Jul 31, 2025
 *
 * Usage Notes:
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

namespace BranchCollision
{
    bool checkBranchCollision(Branch &branch,
                              const std::vector<Branch> &allBranches,
                              const SimulationParameters& params);

  bool checkBoundaryCollision(const Branch &branch,
                                const SimulationParameters &params,
                                const Vec3 BoxCenter,
                                const Vec3 BoxWidth,
                                const double soft_length);


  bool checkSomaCollision(const Branch &branch,
                                const SimulationParameters &params);
}
