/******************************************************************************
 * File:        BranchSpawn.cpp
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module generates new branches through spawning (side branching and bifurcation) new branches.
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License (or specify another if applicable).
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
#include "Utilities.h"
#include "RandomUtils.h"
#include "SimulationParameters.h"
#include "Branch/BranchCommon.h"
#include "Branch/BranchSpawn.h"
////// copy neighbors ////
inline void copyAndOverwritePrimaryNeighbors(Branch& dst, const Branch& src)
{
    dst.Neighbors = src.Neighbors;
    if (dst.Neighbors.size() < 3)
        dst.Neighbors.resize(3, -1);

    dst.Neighbors[0] = dst.ID;
    dst.Neighbors[1] = (dst.Parent_ID  >= 0) ? dst.Parent_ID  : -1;
    dst.Neighbors[2] = (dst.Sibling_ID >= 0) ? dst.Sibling_ID : -1;
}

///////////////// side Branching //////////
inline bool InDeletionList(int ID, std::vector<int> &Deletion_List)
    {
        return std::find(Deletion_List.begin(), Deletion_List.end(), ID) != Deletion_List.end();
    }

bool BranchSpawn::checkNascentCollision(const Vec3& xf,
                                  const Branch &parentbranch,
                                  const std::vector<Branch> &allBranches,
                                  const SimulationParameters& params)
{
        BoundingBox curr_tip;
        const double threshold=0.01;
        const double lini= params.NascentBranchLength;
        const double lint= params.Interaction_Threshold;
        int N=static_cast<int>(allBranches.size());

        curr_tip.expandToInclude(Vec3{xf.x - threshold, xf.y - threshold, xf.z - threshold});
        curr_tip.expandToInclude(Vec3{xf.x + threshold, xf.y + threshold, xf.z + threshold});

    for (int idx : parentbranch.Neighbors){
        if(idx<0 || idx>=N) return 0;

        const Branch& other = allBranches[idx]; 

        const size_t ns = other.Points.size();

        if (ns == 0){ continue;}

        if(lint>lini && other.ID == parentbranch.ID){continue;} // Skip parent branch
        
        if (!curr_tip.intersects(other.Bbox)){continue;} // No bounding box intersection means no collision
       
        const double r1 = other.Radius;
        const double rr = r1 * r1;  

        for (size_t k = 0; k < ns; ++k ){
        if (xf.calculateDistanceSqr(other.Points[k])<= rr){//Current tip is within radius of other branch
            return true; 
        }
        }
                
    }

    return false; // No collision detected
}


void BranchSpawn::spawnBranch(int branch_id,
                    std::vector<Branch> &allBranches,
                    const SimulationParameters &params,
                    const double branching_rate,
                    const std::vector<double> &StateVelocity,
                    std::vector<int> &Deletion_List)
    {
        Branch &branch = allBranches[branch_id];

        bool freeze=(params.FreezeCollidedBranch)? branch.Collision:false;

        int np = branch.Points.size();
        
        const int pid=branch.Parent_ID;
        double rleft=(pid>=0)? allBranches[pid].Radius : params.R_Soma;
       
        const int cid=branch.Child2_ID;
        double rright=(cid>=0)? allBranches[cid].Radius : params.MinimumRadius;
      
        double l_th = rleft + rright  + 2.0*params.MinimumRadius + 4.0*params.BranchInterval;
        //int nth= std::ceil((rleft + rright  + 2.0*params.MinimumRadius)/params.BranchInterval) + 1;

        if (InDeletionList(branch.ID, Deletion_List) || branch.Length <= l_th || freeze || branch.Termination){
        return;
        } 

        double branch_prob =(params.LengthDependentBranching)? 1.0 - exp(-branching_rate * params.Dt* branch.Length):1.0 - exp(-branching_rate * params.Dt);
        if (RandomUtils::UnitUniform() >= branch_prob){
            return;
        }

        int initial_ind = std::ceil(rleft/ params.BranchInterval + 1 );
        int final_ind = (np -1)- std::ceil(rright / params.BranchInterval);


        if(final_ind<initial_ind)
        return;


        int base_idx = RandomUtils::IntUniform(initial_ind,final_ind);

        Vec3 base_pt = branch.Points[base_idx];
        Vec3 base1_pt = branch.Points[base_idx + 1];

        ////if base is with soma return
        double dist_soma=base_pt.calculateDistanceSqr(params.Root);
        double rsq=params.R_Soma*params.R_Soma;

        if(dist_soma<rsq)
        return;

        // Split points
        std::vector<Vec3> points_base(branch.Points.begin(), branch.Points.begin() + base_idx + 1);
        std::vector<Vec3> points_top(branch.Points.begin() + base_idx + 1, branch.Points.end());
        // New branch IDs
        int top_id = allBranches.size();
        int new_id = top_id + 1;

        // === New Branch 1 (Top Half of Original) ===
        Branch new_branch1;
        new_branch1.Points = std::move(points_top);
        new_branch1.BasePoint = base_pt;
        new_branch1.State = branch.State;
        new_branch1.Age = branch.Age;
        new_branch1.ID = top_id;
        new_branch1.Parent_ID = branch_id;
        new_branch1.Child1_ID = branch.Child1_ID;
        new_branch1.Child2_ID = branch.Child2_ID;
        new_branch1.Sibling_ID = new_id;
        new_branch1.Radius = branch.Radius;
        new_branch1.Velocity = branch.Velocity;
        new_branch1.Dynamic = branch.Dynamic;
        new_branch1.Initial = false;
        new_branch1.Termination=false;
        new_branch1.Collision = branch.Collision;
        new_branch1.PostCollision_Time = branch.PostCollision_Time;
        new_branch1.Deletion = branch.Deletion;
        new_branch1.Length = new_branch1.calculateContourLength();
        new_branch1.LastDist = new_branch1.calculateLastDist();
        copyAndOverwritePrimaryNeighbors(new_branch1, branch);



        // === New Branch 2 (Nascent Side Branch) ===
        Branch new_branch2;
        new_branch2.BasePoint = base_pt;
        new_branch2.State = 0;
        new_branch2.Age = 0.0;
        new_branch2.ID = new_id;
        new_branch2.Parent_ID = branch_id;
        new_branch2.Child1_ID = -1;
        new_branch2.Child2_ID = -1;
        new_branch2.Sibling_ID = top_id;
        new_branch2.Radius = params.MinimumRadius;
        new_branch2.Velocity =StateVelocity[new_branch2.State];
        new_branch2.Dynamic = true;
        new_branch2.Initial = false;
        new_branch2.Collision = false;
        new_branch2.PostCollision_Time = 0.0;
        new_branch2.Deletion = false;
        new_branch2.Termination=false;
        copyAndOverwritePrimaryNeighbors(new_branch2, branch);



        Vec3 new_dir;
        double phi, theta;

    // Lambda to compute azimuthal and polar angles
        auto get_phi = [&]() -> double {    
            if (params.AzimuthalAngleMode == "uniform") return RandomUtils::Uniform(0.0, 2.0 * M_PI);
             double ang= RandomUtils::Normal(params.AzimuthalBranchingAngle,params.BranchingAngleSTD);
            if (params.AzimuthalAngleMode == "fixed")   return ang;
            return (RandomUtils::UnitUniform()>0.5)? ang : 2.0*M_PI-ang;
        };

        auto get_theta = [&]() -> double {
            if (params.PolarAngleMode == "uniform") return RandomUtils::Uniform(0.0, M_PI);
             double ang= RandomUtils::Normal(params.PolarBranchingAngle,params.BranchingAngleSTD);
            if (params.PolarAngleMode == "fixed")   return ang;
            return RandomUtils::RandomSign() * ang;
        };

        phi = get_phi();
        theta = get_theta();
        
        if (params.Dimension == 2) theta = M_PI / 2.0;

        if (params.BranchingAngleRef == "global") {
            new_dir = {
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                AdjustTol(cos(theta))
            };
        new_dir.normalize();
        } else {
            Vec3 tangent = (base1_pt - base_pt).normalized();
            if (params.Dimension == 2) {
                double angle = atan2(tangent.y, tangent.x) + phi;
                new_dir = { cos(angle), sin(angle), 0.0 };
                new_dir.normalize();
            } else {
                Vec3 localDir = Vec3::sampleLocalSpherical(theta, phi);
                new_dir = Vec3::rotateZTo(localDir, tangent);
        }
    }


        Vec3 newpt = base_pt;
        double len = params.BranchInterval;
        double target_length = params.NascentBranchLength + branch.Radius;
        bool col2=false;
        while (len < target_length)
        {
            newpt += new_dir * params.BranchInterval;
            if (params.Dimension == 2) newpt.z = 0.0;

            new_branch2.Points.emplace_back(newpt);

            if(len>branch.Radius + params.MinimumRadius) {
                col2=checkNascentCollision(new_branch2.Points.back(), branch, allBranches,params);        
                if(col2) break;       
            } 

            len += params.BranchInterval;
        }

        new_branch2.Length =len;
        new_branch2.LastDist=new_branch2.calculateLastDist();

        // Check for nascent branch collision
        //col2=checkNascentCollision(new_branch2.Points.back(), branch, allBranches,params,sorted_indices,sort_axis);
        if(!col2){///if nascent branches do not collide
        // === Update original branch ===
        if (!branch.Dynamic)
        {
            if (branch.Child1_ID >= 0 && branch.Child1_ID < static_cast<int> (allBranches.size()))
                allBranches[branch.Child1_ID].Parent_ID = top_id;
            if (branch.Child2_ID >= 0 && branch.Child2_ID < static_cast<int> (allBranches.size()))
                allBranches[branch.Child2_ID].Parent_ID = top_id;
        }

    
        branch.Velocity = 0.0;
        branch.Dynamic = false;
        branch.Collision = false;
        branch.Child1_ID = top_id;
        branch.Child2_ID = new_id;
        branch.PostCollision_Time = 0.0;
        branch.Points = std::move(points_base);
        branch.Length = branch.calculateContourLength();
        branch.LastDist=branch.calculateLastDist();
        branch.updateBoundingBox();
        branch.Neighbors.clear();

        // === Add newly formed branches ===
        allBranches.emplace_back(std::move(new_branch1));
        allBranches.back().updateBoundingBox();

        allBranches.emplace_back(std::move(new_branch2));
        allBranches.back().updateBoundingBox();
  
        }

    }


////////////////////////////////////////////////////////////////
/////////////////  Bifurcation //////////  ///////////////////
///////////////////////////////////////////////////////////////
void BranchSpawn::bifurcateBranch(int branch_id, std::vector<Branch> &allBranches,
                        const SimulationParameters &params,
                        const double branching_rate,
                        const std::vector<double> &StateVelocity,
                        std::vector<int> &Deletion_List)
    {
        Branch &branch = allBranches[branch_id];

        bool freeze=(params.FreezeCollidedBranch)? branch.Collision:false;

        const int pid=branch.Parent_ID;
        double rleft=(pid>=0)? allBranches[pid].Radius : params.R_Soma;
      
        double l_th = rleft + params.MinimumRadius  + 2.0*params.BranchInterval;
        
        if (InDeletionList(branch.ID, Deletion_List) || branch.Length <= l_th || !branch.Dynamic || freeze || branch.Termination) //|| branch.MinDisSqr<params.NascentBranchLength*params.NascentBranchLength
            return;


       double branch_prob =(params.LengthDependentBranching)? 1.0 - exp(-branching_rate * params.Dt*branch.Length):1.0 - exp(-branching_rate* params.Dt);
        if (RandomUtils::UnitUniform() >= branch_prob)
            return;


        Vec3 base_pt = branch.Points.back();
        Vec3 base1_pt = branch.Points[branch.Points.size() - 2];


        // New branch IDs
        int first_id = allBranches.size();
        int second_id = first_id + 1;

        // === New Branch 1 (Top Half of Original) ===
        Branch new_branch1;
        new_branch1.BasePoint = branch.Points.back();
        new_branch1.State = 0 ;
        new_branch1.Age = 0.0;
        new_branch1.ID = first_id;
        new_branch1.Parent_ID = branch_id;
        new_branch1.Child1_ID = -1;
        new_branch1.Child2_ID = -1;
        new_branch1.Sibling_ID =second_id;
        new_branch1.Radius = params.MinimumRadius;
        new_branch1.Velocity = StateVelocity[new_branch1.State] ;
        new_branch1.Dynamic = true;
        new_branch1.Initial = false;
        new_branch1.Collision = false;
        new_branch1.PostCollision_Time = 0.0;
        new_branch1.Deletion = false;
        new_branch1.Termination=false;
        copyAndOverwritePrimaryNeighbors(new_branch1, branch);
        
        Vec3 new_dir;
        double phi, theta1,theta2;

    // Lambda to compute azimuthal and polar angles
        auto get_phi = [&]() -> double {    
            if (params.AzimuthalAngleMode == "uniform") return RandomUtils::Uniform(0.0, 2.0 * M_PI);
             double ang= std::abs(RandomUtils::Normal(params.AzimuthalBranchingAngle,params.BranchingAngleSTD));
            if (params.AzimuthalAngleMode == "fixed")   return ang;
            return (RandomUtils::UnitUniform()>0.5)? ang : 2.0*M_PI-ang;
        };

        auto get_theta = [&]() -> double {
            if (params.PolarAngleMode == "uniform") return RandomUtils::Uniform(0.0, M_PI);
            double ang= std::abs( RandomUtils::Normal(params.PolarBranchingAngle,params.BranchingAngleSTD));
            return std::fmod(ang,2.0*M_PI);
        };

        phi = get_phi();

        static double r  = params.BifurcationRatio;
        static double f1 = 1.0 / (1.0 + r);
        static double f2 = r   / (1.0 + r);
        double theta = get_theta();

       
        const bool swapAngles =
        (params.PolarAngleMode == "reflective") &&
        (RandomUtils::UnitUniform() > 0.5);

        theta1 = theta * (swapAngles ? f1 : f2);
        theta2 = theta * (swapAngles ? f2 : f1);    
      

        Vec3 tangent = (base_pt - base1_pt).normalized();

        if (params.Dimension == 2 || params.AzimuthalAngleMode == "2d") {
                double angle = atan2(tangent.y, tangent.x) + theta1;
                new_dir = { cos(angle), sin(angle), 0.0 };
                new_dir.normalize();
        } else {
                Vec3 localDir = Vec3::sampleLocalSpherical(theta1, phi);
                new_dir = Vec3::rotateZTo(localDir, tangent);
        }


        Vec3 newpt = base_pt;
        double len = params.BranchInterval;
        double target_length = params.NascentBranchLength + branch.Radius;
        while (len < target_length)
        {
            newpt += new_dir * params.BranchInterval;
            if (params.Dimension == 2) newpt.z = 0.0;

            new_branch1.Points.emplace_back(newpt);
            len += params.BranchInterval;
        }

        new_branch1.Length =len;
        new_branch1.LastDist=new_branch1.calculateLastDist();
        // === New Branch 2 (Nascent 2nd Branch) ===
        Branch new_branch2;
        new_branch2.BasePoint = branch.Points.back();
        new_branch2.State = 0;
        new_branch2.Age = 0.0;
        new_branch2.ID = second_id;
        new_branch2.Parent_ID = branch_id;
        new_branch2.Child1_ID = -1;
        new_branch2.Child2_ID = -1;
        new_branch2.Sibling_ID = first_id;
        new_branch2.Radius = params.MinimumRadius;
        new_branch2.Velocity = StateVelocity[new_branch2.State];
        new_branch2.Dynamic = true;
        new_branch2.Initial = false;
        new_branch2.Collision = false;
        new_branch2.PostCollision_Time = 0.0;
        new_branch2.Deletion = false;
        new_branch2.Termination=false;
        copyAndOverwritePrimaryNeighbors(new_branch2, branch);


        // Generate nascent branch points
        bool col1=false;
        bool col2=false;

        if (params.Dimension == 2 || params.AzimuthalAngleMode == "2d") {
                double angle = atan2(tangent.y, tangent.x) - theta2;
                new_dir = { cos(angle), sin(angle), 0.0 };
                new_dir.normalize();
        } else {
                Vec3 localDir = Vec3::sampleLocalSpherical(-theta2, phi);
                new_dir = Vec3::rotateZTo(localDir, tangent);
        }

        newpt = base_pt;
        len = params.BranchInterval;
        target_length = params.NascentBranchLength + branch.Radius;
        while (len < target_length)
        {
            newpt += new_dir * params.BranchInterval;
            if (params.Dimension == 2) newpt.z = 0.0;

            new_branch2.Points.emplace_back(newpt);


            if(len>branch.Radius + params.MinimumRadius) {
                col1=checkNascentCollision(new_branch1.Points.back(), branch, allBranches,params);
                col2=checkNascentCollision(new_branch2.Points.back(), branch, allBranches,params);
                if(col1 || col2){
                    break;
                }
            } 



            len += params.BranchInterval;
        }

        new_branch2.Length =len;
        new_branch2.LastDist=new_branch2.calculateLastDist();

       if(!col1 && !col2){///if nascent branches do not collide implement barnching
        branch.Velocity = 0.0;
        branch.Dynamic = false;
        branch.Collision = false;
        branch.Child1_ID = first_id;
        branch.Child2_ID = second_id;
        branch.PostCollision_Time = 0.0;
        branch.Length = branch.calculateContourLength();
        branch.LastDist=branch.calculateLastDist();
        branch.Neighbors.clear();
        allBranches.back().updateBoundingBox();
        // === Add newly formed branches ===
        
        allBranches.emplace_back(std::move(new_branch1));
        allBranches.back().updateBoundingBox();

        allBranches.emplace_back(std::move(new_branch2));
        allBranches.back().updateBoundingBox();

    }
       
    }