/******************************************************************************
 * File:        BranchForces.h
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module calculates the beanding and streaching forces
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
#include "BranchCommon.h"

namespace BranchForces
{

void updatePosition_ImplicitGlobal(std::vector<Branch>& allBranches,
                                                 const SimulationParameters& params,const double DT_multiplier);

                      
};
