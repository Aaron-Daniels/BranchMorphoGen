/******************************************************************************
 * File:        Simulation.cpp
 * Project:     Branching Morphogenesis Generator (BranchMorphoGen)
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module is the meat of the simulation. It sequentially updates
      the fundamental process such as, branching, elongation/retraction, straightening etc
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Dec 02, 2025
 *  
 * Usage Notes:
 *   - All units are user provided. (default is microns and minutes)
 *   - Requires C++17 or later.
 *   - Dependencies: This codebase needs CMake, libtiff
 *
 * References:
 *   [1] Shree, Sutradhar, et al., Sci. Adv. 8, eabn0080 (2022) 29 June 2022
 *   [2] Ouyang, Sutradhar et al.  Nature Comm. 
 *
 ******************************************************************************/
#include <iostream>                // For standard I/O operations
#include <sstream>                 // For string stream manipulations
#include <fstream>                 // For file I/O
#include <cmath>                   // For mathematical functions like sin, cos, M_PI
#include <unordered_map>   // For std::unordered_map
#include <unordered_set>
#include <vector>          // For std::vector
#include <filesystem>
#include <set>             // (if using std::set somewhere)s
#include "RandomUtils.h"          // Custom header for random number utilities
#include "Utilities.h"            // Common utility functions (e.g., AdjustTol)
#include "SimulationParameters.h" // Struct for reading and storing simulation parameters

#include "PrintSWC.h"             // For writing .swc output files
#include "PrintTIFF.h"            // For generating TIFF images
#include "PrintBranchStats.h"      // For printing Branch statistics
#include "PrintTime.h"
#include "Branch/BranchSpawn.h"   // Functions to spawn/bifurcate branches
#include "Branch/BranchGrowth.h"  // Functions for updating branch growth and state
#include "Branch/BranchRelabel.h" // Handles debranching and reindexing
#include "Branch/BranchForces.h"  // Applies mechanical forces to branches
#include "Branch/BranchTopology.h"
#include "ReadSWC.h"
#include "Simulation.h"
#include "Banner.h"
namespace fs = std::filesystem;
/// Initialization ////////////////////////////////////
Simulation::Simulation(std::string& swcFiles, SimulationParameters &params)
{

    if(params.ReadFromSWC){

    Branches = SWCParser::SWCTreeParser::parse(swcFiles,params);

    }
    else{
    params.N_BranchInitial=RandomUtils::IntUniform(params.N_BranchInitialRange.first,params.N_BranchInitialRange.second);
    int NInitialBranches = params.N_BranchInitial; // Number of initial branches
    
    Branches.clear();
    Branches.resize(NInitialBranches); // Reserve space for initial branches

    double angle_increment = 2.0 * M_PI / static_cast<double>(NInitialBranches); // Uniform angular spacing for initial branches

    for (int i = 0; i < NInitialBranches; ++i)
    {
        double phi = params.InitialAngle + angle_increment * i; // Azimuthal angle
        double theta = (params.Dimension == 2) ? M_PI / 2.0 : params.InitialAngleWithZ; // Elevation angle

        Branch &B = Branches[i];
        B.BasePoint = params.Root;
        B.State = 0;
        B.ID = i;
        B.Parent_ID = -1;
        B.Child1_ID = -1;
        B.Child2_ID = -1;
        B.Sibling_ID = -1;
        B.Radius = params.MinimumRadius;
        std::vector<double> velocity=params.StateVelocity.getValue(params.Time_Start);
        B.Velocity = velocity[0]; // Assign initial velocity from state
        B.Dynamic = true;
        B.Initial = true;
        B.Collision = false;
        B.PostCollision_Time = 0.0;
        B.Deletion = false;
        B.Termination=false;
        B.Length = params.BranchInterval;

        int j = 0;
        while (B.Length <= params.R_Soma + params.InitialLength)
        {
            j++;
            // Compute displacement in spherical coordinates
            double dx = j * params.BranchInterval * sin(theta) * cos(phi);
            double dy = j * params.BranchInterval * sin(theta) * sin(phi);
            double dz = AdjustTol(j * params.BranchInterval * cos(theta));
            if (params.Dimension == 2) {dz = 0.0;} // Flatten to XY plane for 2D

            B.Length += params.BranchInterval;
            B.Points.push_back({params.Root.x + dx, params.Root.y + dy, params.Root.z + dz});
        }
        B.updateBoundingBox(); // Update spatial bounding box of the branch
        B.LastDist=B.calculateLastDist();
    }

}

}

std::string Simulation::openTimeFile(SimulationParameters& params, int sample) {
    std::ostringstream oss;
    oss << "TemproralProps-" << sample;
    std::string filename = params.SimulationName + "-" + oss.str() + ".csv";

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write Temporal properties file: " + filename);
    }

    file << std::setprecision(4) << std::fixed;
    file << BranchMorphoGen::HASHBANNER;
    file << "#Simulation name: " << params.SimulationName << "\n";
    file << "#Info=All lengths are in "<<params.SpatialUnit<<" & "<<params.TemporalUnit<<std::endl;
    file << "#Tree properties at each recorded time point\n";

    file << "#COLUMN_NAMES:" << std::endl;
    file << "time("
    <<params.TemporalUnit<<"),"
    << "# of branches, # of tips, Total length("
    <<params.SpatialUnit<<"), "
    <<"Avg length("
    <<params.SpatialUnit<<"), "
    << std::endl;

    return filename;
}
/*
int Simulation::sortAxes(const int N, SimulationParameters& params)
{
      int sortAxis = 0;

    if(params.Dimension>2){ /// check longest axis in 3d only
    BoundingBox globalBBox = Branches[0].Bbox;
    for (int i = 1; i < N; i+=5)
    {
        globalBBox.expandToInclude(Branches[i].Bbox);
    }

    // Step 1: Determine longest axis
    Vec3 extent = globalBBox.max - globalBBox.min;
    if (extent.y > extent.x) sortAxis = 1;
    if (extent.z > extent[sortAxis]) sortAxis = 2;
    }

    return sortAxis;
}

*/
/// UPdate neighborlist ////////


void Simulation::updateNeighborList(std::vector<Branch> &branches, 
                                    SimulationParameters& params)
{
    const int N = static_cast<int>(branches.size());
    if (N == 0) return;

    int sortAxis = 0;

    if(params.Dimension>2){ /// check longest axis in 3d only
    BoundingBox globalBBox = branches[0].Bbox;
    for (int i = 1; i < N; i+=5)
    {
        globalBBox.expandToInclude(branches[i].Bbox);
    }

    // Step 1: Determine longest axis
    Vec3 extent = globalBBox.max - globalBBox.min;
    if (extent.y > extent.x) sortAxis = 1;
    if (extent.z > extent[sortAxis]) sortAxis = 2;
    }

    // Step 2: Precompute sorted indices by Bbox.min.{axis}
    std::vector<int> sorted_indices(N);
    std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
    std::sort(sorted_indices.begin(), sorted_indices.end(),
              [&](int a, int b) {
                  return branches[a].Bbox.min[sortAxis] < branches[b].Bbox.min[sortAxis];
              });

    double padding=params.Interaction_Threshold+params.CollisionPadding;
// Step 3: Loop through each dynamic branch and build neighbor list
    for (int i = 0; i < N; ++i)
    {
        auto &b1 = branches[i];

        if (!b1.Dynamic) {
            b1.Neighbors.clear();
            continue;
        }

        // Clear and reserve
        b1.Neighbors.clear();
        b1.Neighbors.reserve(8);

        b1.Neighbors.emplace_back(i);
        if (b1.Parent_ID >= 0)  b1.Neighbors.emplace_back(b1.Parent_ID);
        if (b1.Sibling_ID >= 0) b1.Neighbors.emplace_back(b1.Sibling_ID);

        BoundingBox padded = b1.Bbox;
        padded.expand(padding);

        for (int idx : sorted_indices)
        {
            if (idx == i || idx == b1.Parent_ID || idx == b1.Sibling_ID) continue;

            // Early exit: bounding boxes are sorted along the chosen axis
            if (branches[idx].Bbox.max[sortAxis] < padded.min[sortAxis]) continue;
            if (branches[idx].Bbox.min[sortAxis] > padded.max[sortAxis]) break;

            if (padded.intersects(branches[idx].Bbox)) {
                b1.Neighbors.emplace_back(idx);
            }
        }
    }  

}


void Simulation::Step(SimulationParameters &params,
                    const size_t irun,
                    const int iteration_index,
                    const double tsim,std::string filename)
{

    ///////////// Store dynamic parameters at current time /////////////
    const auto current_velocities = params.StateVelocity.getValue(tsim);
    const double growth_velocity= current_velocities[0]; // Default growth velocity for state 0
    const auto current_transition_rates = params.TransitionRates.getValue(tsim);
    const auto current_branching_rate = params.BranchingRate.getValue(tsim);  
    params.updateBoundary(tsim);
    ///////////////. fix boundary parameters
     const double soft_length=params.Boundary_SoftLength;
    const Vec3 BoxCenter = (params.Boundary.max + params.Boundary.min) * 0.5;
    const Vec3 BoxWidth = (params.Boundary.max - params.Boundary.min) * 0.5 + Vec3{soft_length,soft_length,soft_length}*2.0;
  
    /////////////////////// Old stuff
    size_t old_size = Branches.size();
    std::vector<int> Deletion_List;
    Deletion_List.clear();
    //////. Update neighborlist
        if(irun%params.CollisionSkip==0 ){
        updateNeighborList(Branches,params);
    }

    ////////////////////// Tip Dynamics at each step
    for (size_t i = 0; i < old_size; ++i)
        {   
            Branch &B = Branches[i];
            B.updateAge(params,growth_velocity); // Increase age of tip
            B.updateTermination(params);
            B.Length= B.calculateContourLength();
            B.LastDist=B.calculateLastDist();
            BranchGrowth::elogationDynamics(B,Branches,Deletion_List,params,current_transition_rates,current_velocities,BoxCenter,BoxWidth,soft_length); 
        }


        if (params.Bifurcation){
        for (size_t i = 0; i < old_size; ++i)
        {   
        BranchSpawn::bifurcateBranch(i, Branches, params,current_branching_rate,current_velocities,Deletion_List);// bifurcate based on setting
        }
        }else{
         for (size_t i = 0; i < old_size; ++i)
        {   
            BranchSpawn::spawnBranch(i, Branches, params,current_branching_rate,current_velocities,Deletion_List); // Spawn new lateral branches 
        }
        }
    //////////////// If there is debranching fix tree topology and update neighborlist
    keypointTracker_.assignAfterBranching(Branches);
    BranchRelabel::debranching(Branches, Deletion_List, params);
    keypointTracker_.reconcileAfterTopology(Branches);
    /////////////////////////// Calculate force and update positions and radius
    if (irun % params.ForceSkip == 0){
        BranchTopology::updateRadius(Branches, params);   
        if (params.StraightenBranches){ 
        BranchForces::updatePosition_ImplicitGlobal(Branches, params, params.ForceSkip);
        }   
        }
    
    ///// Print overall time properties
    if(irun % static_cast<int>(1.0 / params.Dt) == 0) {PrintTimeFile::printTimeFile(Branches, filename, tsim);}

    keypointTracker_.recordLifecycle(Branches, irun, tsim);

    /////////////////////////// Print swc and tiff files
    if (irun % params.SWC_frequency == 0)
    {
        keypointTracker_.writeSnapshot(Branches, irun, tsim);
        PrintSWC::printSWCFiles(Branches, params, iteration_index, tsim);
        PrintTIFF::printImage(Branches, params, iteration_index, tsim);
        PrintBranchStats::printBranchStats(Branches, params, iteration_index, tsim);
        //std::cout << "Printing SWC and TIFF files at time " << tsim << std::endl;
    }
}

void Simulation::Run(SimulationParameters &params, const int iteration_index)
{


    auto timefile=openTimeFile(params,iteration_index);
    double tsim = params.Time_Start; // Simulation time
    size_t istep = 0;
    ///////////////////////////////
    updateNeighborList(Branches,params);
    keypointTracker_.initialize(Branches, params, iteration_index);
    ///////////////////////////////////
    while (tsim <= params.Time_End + params.Dt)
    {
        
        Step(params, istep, iteration_index, tsim, timefile); // Perform one step

        tsim += params.Dt;                      // Increment time
        istep++;                                // Step index

       
    }


}
