
/******************************************************************************
 * File:        BranchCollision.cpp
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *  This header conatians all common branch structure
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

#include <cstdint>
#include <vector>
#include "Utilities.h"
#include "SimulationParameters.h"
struct Branch
{
    double Age;
    std::vector<Vec3> Points;
    std::vector<int> Neighbors;
    Vec3 BasePoint;
    int State;
    int ID;
    int Parent_ID;
    int Child1_ID;
    int Child2_ID;
    int Sibling_ID;
    // Immutable, sample-scoped identities for temporal keypoint export.
    std::int64_t PersistentTipID = -1;
    std::int64_t PersistentJunctionID = -1;
    double Radius;
    double Velocity;
    bool Dynamic;
    bool Initial;
    bool Collision;
    bool Termination;
    double PostCollision_Time;
    double LastDist;
    bool Deletion;
    BoundingBox Bbox;
    double Length;


    double calculateContourLength() const;
    double calculateLastDist() const;
    void updateAge(const SimulationParameters &params,const double growth_velocity);
    void updateBoundingBox();
    void updateTermination(const SimulationParameters &params);
    

};
