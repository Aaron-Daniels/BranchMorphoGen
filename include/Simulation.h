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
 * Last Modified:    Jul 23, 2025
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
#ifndef SIMULATION_H
#define SIMULATION_H
#include <vector>
#include "SimulationParameters.h"
#include "Branch/BranchCommon.h"
#include "KeypointTracker.h"

class Simulation
{
public:
    Simulation(std::string& swcFiles,SimulationParameters &params);

    std::string openTimeFile(SimulationParameters& params,
                             const int sample);

   // int sortAxes(const int N, SimulationParameters &params);

   void updateNeighborList(std::vector<Branch> &branches, 
                                    SimulationParameters& params);

    void Step(SimulationParameters &params,
              const size_t irun,
              const int iteration_index,
              const double tsim,
              std::string filename);

    void Run(SimulationParameters &params,
             const int iteration_index);

private:
    std::vector<Branch> Branches;
    KeypointTracker keypointTracker_;

    /* ---------- Added ---------- */
    //std::vector<int> sorted_indices;   // persistent spatial ordering
    //int sort_axis = 0;                 // last chosen sorting axis
    /* --------------------------- */
};

#endif
