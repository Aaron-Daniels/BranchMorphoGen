/******************************************************************************
 * File:        BranchSpawn.h
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
   This header file generates new branches through spawning (side branching and bifurcation) new branches.
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License (or specify another if applicable).
 *
 * Last Modified:    Jul 1, 2025
 *
 * Usage Notes:
 *   - Requires C++17 or later.
 *   - Dependencies: This codebase needs CMake, libtiff
 *
 * References:
 *   [1] Shree, Sutradhar, et al., Sci. Adv. 8, eabn0080 (2022) 29 June 2022
 *   [2] Ouyang, Sutradhar et al., Nature Comm. 2025
 *
 ******************************************************************************/
#pragma once
#include <iostream>
#include "SimulationParameters.h"
#include "Branch/BranchCommon.h"

namespace BranchSpawn
{

  bool checkNascentCollision(const Vec3& xf,
                                  const Branch &parentbranch,
                                  const std::vector<Branch> &allBranches,
                                  const SimulationParameters& params);

  void spawnBranch(int branch_id, std::vector<Branch> &allBranches,
                    const SimulationParameters &params,
                    const double branching_rate,
                    const std::vector<double> &StateVelocity,
                    std::vector<int> &Deletion_List);

  void bifurcateBranch(int branch_id, std::vector<Branch> &allBranches,
                        const SimulationParameters &params,
                        const double branching_rate,
                        const std::vector<double> &StateVelocity,
                        std::vector<int> &Deletion_List);
};
