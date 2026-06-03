/******************************************************************************
 * File:        BranchForces.h
 * Project:     Branching Morphogenesis Generator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module Prints .swc files
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Jul 23, 2025
 *
 * Usage Notes:
 *   - All units are in microns and seconds unless otherwise specified.
 *   - Requires C++17 or later.
 *   - Dependencies: This codebase needs CMake, libtiff
 *
 * References:
 *   [1] Shree, Sutradhar, et al., Sci. Adv. 8, eabn0080 (2022) 29 June 2022
 *   [2] Ouyang, Sutradhar et al.  Nature Comm. 
 ******************************************************************************/

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "PrintSWC.h"
#include "Banner.h"

void PrintSWC::connectPoints(const std::string& filename,
                   std::vector<Branch>& allBranches,
                   int branchIndex,
                   std::vector<int>& parentSWC_ID,
                   int& NTotalPoints) {
    
    Branch& branch = allBranches[branchIndex];

    std::ofstream file(filename, std::ios::app);

    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }


    for (size_t j = 0; j < branch.Points.size(); ++j) {
        const Vec3& point = branch.Points[j];
        NTotalPoints++;

        if (j == 0){
            file << NTotalPoints << " " << 4 << " "
             << std::fixed << std::setprecision(3)
             << point.x << " " << point.y << " " << point.z << " "
             << branch.Radius << " "
             << parentSWC_ID[branchIndex] << "\n";

        }else{
            file << NTotalPoints << " " << 4 << " "
             << std::fixed << std::setprecision(3)
             << point.x << " " << point.y << " " << point.z << " "
             << branch.Radius << " "
             <<  NTotalPoints-1 << "\n";
        }


    }

    file.close();

    if (!branch.Dynamic && branch.Child1_ID >= 0 && branch.Child2_ID >= 0) {
         parentSWC_ID[branch.Child1_ID]=NTotalPoints;
         parentSWC_ID[branch.Child2_ID]=NTotalPoints;
        connectPoints(filename, allBranches, branch.Child1_ID, parentSWC_ID, NTotalPoints);
        connectPoints(filename, allBranches, branch.Child2_ID, parentSWC_ID, NTotalPoints);
    }
}

void PrintSWC::printSWCFiles(std::vector<Branch>& allBranches, const SimulationParameters& params, const int sample, const double& tsim) {
    std::ostringstream oss;
    oss << "Conf-" << sample << "-time-" << tsim;
    std::string filename = params.SimulationName + "-" + oss.str() + ".swc";

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write SWC file");
    }

    // SWC Header
    file << std::setprecision(4) << std::fixed; // control decimal precision
    file << BranchMorphoGen::HASHBANNER;
    file << "#Simulation name: " << params.SimulationName << ".\n";
    file << "#File contains coordinates and connection of branches.\n";
    file << "#Info=All lengths are in "<<params.SpatialUnit<<" & "<<params.TemporalUnit<<std::endl;
    file << "#COLUMN_NAMES:"<<std::endl;
    file << "#ID, Type, x, y, z, radius, ParentID"<<std::endl;
    file << "1 1 "<<params.Root.x<<" "<<params.Root.y<<" "<<params.Root.z<<" "<<0.0<<" -1"<<std::endl;
    file.close();

    int NTotalPoints = 1; // SWC ID 1 is soma
    std::vector<int>parentSWC_ID(allBranches.size(),0);

    for (int i = 0; i < params.N_BranchInitial; ++i) {
        parentSWC_ID[i]=1;
        connectPoints(filename, allBranches, i, parentSWC_ID, NTotalPoints);
    }
}

// namespace PrintSWC

