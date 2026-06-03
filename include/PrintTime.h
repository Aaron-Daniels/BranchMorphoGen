/******************************************************************************
 * File:        BranchTime.h
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
 *   [2] Ouyang, Sutradhar et al.  Nature Comm. 
 *
 ******************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <tiffio.h>
#include <iostream>
#include <stdexcept>
#include "Utilities.h"
#include "Branch/BranchCommon.h"

namespace PrintTimeFile {
  void printTimeFile(std::vector<Branch>& allBranches,const std::string& filename, const double& tsim);

};


