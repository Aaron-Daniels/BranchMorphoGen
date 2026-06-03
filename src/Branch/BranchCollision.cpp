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


#include <iostream>
#include <cmath>
#include <algorithm>
#include "Utilities.h"
#include "SimulationParameters.h"
#include "Branch/BranchCollision.h"

inline double softOutside(double x,const double xc,const double w,const double sigma) noexcept
{
     // Smooth outside indicator in [0,1]
    //const double s = 1.0 - std::exp(-(std::abs(x - xc) - w) / sigma);
    const double s = 0.5 * (1.0 + std::erf((std::abs(x - xc) - w) / sigma));
    // Numerical safety
    return std::clamp(s, 0.0, 1.0);

}




    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check Collision with other branches ////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Checks whether the current branch tip collides with any nearby branches
/*
bool BranchCollision::checkBranchCollision(Branch &branch,
                              const std::vector<Branch> &allBranches,
                              const SimulationParameters& params)
    {

        if (branch.Points.empty() ){return false;} // No points means nothing to check

        const Vec3 &xf = branch.Points.back(); // Tip of the current branch

        // Define a small cubic region (bounding box) around the tip for coarse filtering
        BoundingBox curr_tip;
        double threshold= params.Interaction_Threshold;

        curr_tip.expandToInclude(Vec3{xf.x - threshold, xf.y - threshold, xf.z - threshold});
        curr_tip.expandToInclude(Vec3{xf.x + threshold, xf.y + threshold, xf.z + threshold});

        // Loop through nearby branches (neighbors list)
       // for (int j : branch.Neighbors){    
        for (size_t j = 0; j < allBranches.size(); ++j){

            const Branch &other = allBranches[j];
            // std::cout << "ID =" << branch.ID << " Neighbor ID = " << other.ID << " " << std::endl;
            const size_t ns = other.Points.size(); // Number of points in neighbor branch
            
            if (ns == 0){
                continue;
            }
            if (!curr_tip.intersects(other.Bbox)){
                continue; // No bounding box intersection means no collision
            }
            // Effective interaction distance = sum of radii + margin threshold
            double intersctself2 = threshold*threshold;
            double interact =  branch.Radius + other.Radius + threshold;
            const size_t noth=static_cast<size_t>(2.0*interact/params.BranchInterval);
            double interact2 = interact * interact; // Squared distance for faster comparison

            //std::cout<<"Threshold="<<threshold<<" Total dis="<<interact<< " Sqr dis ="<<interact2<<std::endl;

             // General collision check with all points if no special match
            if (other.ID != branch.ID &&
                other.ID != branch.Parent_ID &&
                other.ID != branch.Sibling_ID){
                if (curr_tip.intersects(other.Bbox)){
                for (size_t k = 0; k < ns; k += 1){
                if (xf.calculateDistanceSqr(other.Points[k])<= interact2){
                    return true; // General external collision
                }
                }  
            }
        }
        else if (branch.Length > 2.0*interact)
        {
            // Check self
            if (other.ID == branch.ID && ns > noth)
            {
                for (size_t k = 0; k < ns - noth; k += 1)
                {
                    if (xf.calculateDistanceSqr(other.Points[k]) <= intersctself2){
                        return true;
                    }
                    }
                       
                }
            
            // Same-parent
            else if (other.ID == branch.Parent_ID && ns > noth)
            {
                for (size_t k = 1; k < ns - noth; k += 1)
                {
                    if ( xf.calculateDistanceSqr(other.Points[k])<= intersctself2){  
                    return true;
                    }
                        
                }
            }
            // Sibling
            else if (other.ID == branch.Sibling_ID && ns > noth)
            {
                for (size_t k = noth; k < ns; k++)
                {
                    if (xf.calculateDistanceSqr(other.Points[k]) <= intersctself2){  
                    return true;
                    }
                       
                }
            }
        }
    }

        return false; // No collision detected
    }

*/


bool BranchCollision::checkBranchCollision(Branch &branch,
                                            const std::vector<Branch> &allBranches,
                                            const SimulationParameters& params)
{
    if (branch.Points.empty()) {
        return false; // No points means nothing to check
    }
    int N=static_cast<int>(allBranches.size());
    const Vec3& xf = branch.Points.back();   // Tip of the current branch
    const double threshold  = params.Interaction_Threshold;
    const double threshold2 = threshold * threshold;

    // Part that depends only on this branch and the global parameters
    const double baseInteract       = branch.Radius + threshold;
    const double invBranchInterval  = 1.0 / params.BranchInterval;

    // Small cubic region around the tip
    BoundingBox currTip;

    if(branch.Neighbors.empty()) return 0;

    for (int idx : branch.Neighbors){

        if(idx<0 || idx>=N) return 0;

        const Branch& other = allBranches[idx]; 

        const std::size_t ns = other.Points.size();
        
        if (ns == 0) {continue;}
                // Effective interaction distance for this pair
        const double interact  = baseInteract + other.Radius;
        const double interact2 = interact * interact;
        const std::size_t noth =
        static_cast<std::size_t> (std::ceil(interact * invBranchInterval));
        
        currTip.expandToInclude(Vec3{xf.x - interact, xf.y - interact, xf.z - interact});
        currTip.expandToInclude(Vec3{xf.x + interact, xf.y + interact, xf.z + interact});

            // Coarse filter by bounding box first
            if (!currTip.intersects(other.Bbox)) {
                continue;
            }

        const bool isSelf    = (other.ID == branch.ID);
        const bool isParent  = (other.ID == branch.Parent_ID);
        const bool isSibling = (other.ID == branch.Sibling_ID);

        // External branches
        if (!isSelf && !isParent && !isSibling) {
            for (std::size_t k = 0; k < ns; ++k) {
                if (xf.calculateDistanceSqr(other.Points[k]) <= interact2) {
                    return true;   // General external collision
                }
            }
        }
        // Self or family branches
        else if (branch.Length > 2.0 * interact && ns > noth) {
            std::size_t start = 0;
            std::size_t end   = ns;

            if (isSelf) {
                // Self: check older points only
                start = 0;
                end = ns - noth;
            }
            else if (isParent) {
                // Parent: skip the root, ignore newest segment
                start = 1;
                end   = ns - noth;
            }
            else if (isSibling) {
                // Sibling: check only the newer part
                start = noth;
                end=ns;
            }

            for (size_t k = start; k < end; ++k) {
                if (xf.calculateDistanceSqr(other.Points[k]) <= threshold2) {
                    return true;
                }
            }
        }
    }
    return false; // No collision detected
}

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////. Check Boundary Collision ////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Checks whether the current tip of the branch is outside simulation bounds
bool BranchCollision::checkBoundaryCollision(
                    const Branch& branch,
                    const SimulationParameters& params,
                    const Vec3 BoxCenter,
                    const Vec3 BoxWidth,
                    const double soft_length)
{
    const Vec3& xf = branch.Points.back();
    // Inside soft boundary but outside hard boundary: probabilistic collision
    if (!params.Boundary.isInside(xf, 0.0)) {
        const double px = softOutside(xf.x,BoxCenter.x,BoxWidth.x,soft_length);
        const double py = softOutside(xf.y,BoxCenter.y,BoxWidth.y,soft_length);
        const double pz = softOutside(xf.z,BoxCenter.z,BoxWidth.z,soft_length);
        const double prob = (params.Dimension <= 2)
                  ? std::max(px, py)
                  : std::max({px, py, pz});

    return RandomUtils::UnitUniform() < prob*params.Dt;
    }else{
        return false;
    }


}

///////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////. Check Collision with Soma ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
bool BranchCollision::checkSomaCollision(const Branch &branch,
                                const SimulationParameters &params)
    {
        const Vec3 &xf = branch.Points.back(); // Current tip position
        double dist_soma=xf.calculateDistanceSqr(params.Root);
        double rsq=params.R_Soma*params.R_Soma;
        if (dist_soma<= rsq)
        {
            return true; // Tip is out of bounds
        }
        else
        {
            return false; // Tip is within bounds
        }
    }

