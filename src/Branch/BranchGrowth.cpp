/******************************************************************************
 * File:        BranchCollision.cpp
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *  This module implement bran transitions and elongation/retraction etc.
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


#include "Utilities.h"
#include "SimulationParameters.h"
#include "Branch/BranchCommon.h"
#include "Branch/BranchCollision.h"
#include "Branch/BranchGrowth.h"


static inline void regrow_branch(Branch &branch,const double growth_velocity)
{
                    branch.Collision=false;
                    branch.PostCollision_Time=0.0;
                    branch.State=0;
                    branch.Velocity = growth_velocity;
}

/////////////////////////////////////////// UPDATE STATE //////////////////////////////////////////////////
// Updates the internal dynamic state of the branch based on stochastic transition rates
void BranchGrowth::updateState(Branch &branch, 
                    const SimulationParameters &params,
                    const std::vector<std::vector<double>> &TransitionRates,
                    const std::vector<double> &StateVelocity,
                    bool &StateChanged)
    {
            int current = branch.State;
            int N;

            //// Check for collision and free dynamics
            // Select the number of states depending on whether the branch is in collision
            N = (branch.Collision) ? params.Collision_NState : params.NState; // total number of states, e.g., 3

            // If invalid state or only one state is defined, no transition is possible
            if (N <= 1 || current < 0 || current >= N)
            return;
        
                

            // Select transition rate matrix depending on collision status
            const std::vector<std::vector<double>> &K = branch.Collision
                                                        ? params.Collision_TransitionRates  
                                                        : TransitionRates;

            // Compute total rate of leaving the current state
            double kt = 0.0;
            for (int j = 0; j < N; ++j)
            {
                if (j != current)
                    kt += K[current][j]; // Sum all outgoing rates
            }

            // Decide probabilistically whether a transition occurs during time step Dt
            if (RandomUtils::UnitUniform() >= 1.0 - std::exp(-kt * params.Dt))
                return;
            
                

            // Generate a random number to decide the next state
            double r = RandomUtils::UnitUniform();
            double cum = 0.0;
            int next = current;

            // Loop over all possible new states and find the one matching the random threshold
            for (int j = 0; j < N; ++j)
            {
                if (j == current)
                    continue;
                cum += K[current][j] / kt; // Normalized cumulative probability
                if (r < cum)
                {
                    next = j; // Transition to state j
                    StateChanged=true;
                    break;
                }
            }

            // Update the current state and set corresponding velocity
            branch.State = next;
            double velocity;
            velocity=(branch.Collision)? params.Collision_StateVelocity[next]:StateVelocity[next];
            branch.Velocity = velocity;
   
            if(branch.Initial && branch.Length<params.R_Soma){
            regrow_branch(branch,StateVelocity[0]);
            }   
     
}

void   BranchGrowth::retractLengthFromTip(std::vector<Branch> &allBranches,
                            Branch &branch, 
                            double length_to_remove,
                            std::vector<int> &Deletion_List,
                            const SimulationParameters &params,
                            const std::vector<double> &StateVelocity)
    {
        length_to_remove = std::abs(length_to_remove);

        const double lfinal = branch.Length - length_to_remove;
        double Ri= (branch.Initial)? params.R_Soma : allBranches[branch.Parent_ID].Radius+ params.BranchInterval;

        if (lfinal > Ri)
        { // If too short, mark for deletion or collapse
            double residual=0.0;
            std::vector<Vec3> points;
            Vec3 last_pt;
            points.push_back(branch.Points[0]);
            double laccu = branch.Points[0].calculateDistance(branch.BasePoint);
            for (size_t i = 1; i < branch.Points.size(); ++i)
            {
                laccu += branch.Points[i].calculateDistance(branch.Points[i - 1]);
                if (laccu < lfinal)
                {
                    residual = lfinal - laccu;
                    points.push_back(branch.Points[i]);
                }
                else
                {
                    last_pt = branch.Points[i];
                    break;
                }
            }

            Vec3 curr_pt = points.back();
            Vec3 dir = curr_pt.calculateDirectionCosine(last_pt);
            Vec3 new_tip = curr_pt + dir * residual;
            points.push_back(new_tip);
            branch.Points = std::move(points); // Efficient move  
        }
        else
        { /////////////////  Deletion /////////////////////
            if (branch.Initial) {
                regrow_branch(branch,StateVelocity[0]);
                }else{
                if(RandomUtils::UnitUniform() < params.RebranchingProb){
                regrow_branch(branch,StateVelocity[0]);
                }else{
                Deletion_List.push_back(branch.ID); // add tip to the deletion queue;
                }
            }
        }
    }


void BranchGrowth::addLengthToTip(Branch &branch,
                                    std::vector<Branch> &allBranches,
                                    double ladd,
                                    std::vector<int> &Deletion_List,
                                    const SimulationParameters &params,
                                    const Vec3 BoxCenter,
                                    const Vec3 BoxWidth,
                                    const double soft_length)
    {       
        ladd = std::abs(ladd);

        int n = branch.Points.size();
        if (n < 2)
            return; // Need at least 2 points to determine direction

       
        bool SelfCollision =false ;
        if (params.SelfAvoiding) {
        SelfCollision = BranchCollision::checkBranchCollision(branch, allBranches,params);
        } 

        bool boundaryCollision=BranchCollision::checkBoundaryCollision(branch, params,BoxCenter,BoxWidth,soft_length);
        bool somaCollision=BranchCollision::checkSomaCollision(branch, params);

        // check  collision occurs
        if (SelfCollision==false && boundaryCollision==false && somaCollision==false)
        {
            int lastindx = n - 1;
            int previndx = n - 2;
            Vec3 last = branch.Points[lastindx];
            Vec3 prev = branch.Points[previndx];

            std::vector<Vec3> points;

            points.assign(branch.Points.begin(), branch.Points.end() - 1); // Keep up to the second last point
            Vec3 prev_dir = last - prev;
            double seg_len = prev_dir.norm();
            // Normalize direction
            Vec3 direction = prev_dir / seg_len;
            // Persistence-controlled angular noise
            Vec3 new_dir;
            if(std::isinf(params.PersistanceLength)){
                new_dir=direction;
            }else{
            double stddev = std::sqrt(2.0*ladd * params.PerisitanceInv);
            double d1, d2, d3;
            d1 = RandomUtils::Normal(0.0, stddev);
            d2 = RandomUtils::Normal(0.0, stddev);
            d3 = (params.Dimension==2) ? 0.0 : RandomUtils::Normal(0.0, stddev);

            Vec3 random_vec = {d1, d2, d3};
            Vec3 perturbation = random_vec - prev_dir * random_vec.dotProduct(prev_dir);
            // Vec3 perturbation = Vec3::randomUnitVector() * stddev;
            //  New perturbed direction
            new_dir = (direction + perturbation).normalized();

            }

            // Total length to add
            double ltotaladd = seg_len + ladd;
            Vec3 current = prev;

            // Segment-wise growth
            while (ltotaladd > EPSILON)
            {
                double step = std::min(ltotaladd, params.BranchInterval);
                current = current + new_dir * step;
                points.push_back(current);
                ltotaladd -= step;
            }
            // Update state
            branch.Points = std::move(points);
  
      
        }else
        {
            // Collision occurred
            branch.Collision = true;
            branch.PostCollision_Time = 0.0;
            branch.Velocity = *std::min_element(params.Collision_StateVelocity.begin(),
                                                params.Collision_StateVelocity.end());
            branch.State = index_of_min(params.Collision_StateVelocity);
        }
    }

void BranchGrowth::updateGrowth(Branch &branch, 
                    std::vector<Branch> &allBranches,
                    std::vector<int> &Deletion_List,
                    const SimulationParameters &params,
                    const std::vector<double> &StateVelocity,
                    const Vec3 BoxCenter,
                    const Vec3 BoxWidth,
                    const double soft_length)
    {

    // Increment of length over this step
    const double dl = branch.Velocity * params.Dt;
    if (dl > EPSILON)
    {
        addLengthToTip(branch, allBranches, dl, Deletion_List, params, BoxCenter, BoxWidth, soft_length);
    }
    else if(dl<-EPSILON)
    {
        // retractLengthFromTip expects a negative dl in your current usage
        retractLengthFromTip(allBranches, branch, dl, Deletion_List, params, StateVelocity);
    }


// Update derived quantities
branch.Length =branch.calculateContourLength();
branch.LastDist = branch.calculateLastDist();
branch.updateBoundingBox();
  
}

 void BranchGrowth::elogationDynamics(Branch &branch, 
                    std::vector<Branch> &allBranches,
                    std::vector<int> &Deletion_List,
                    const SimulationParameters &params,
                    const std::vector<std::vector<double>> &TransitionRates,
                    const std::vector<double> &StateVelocity,
                    const Vec3 BoxCenter,
                    const Vec3 BoxWidth,
                    const double soft_length)
{

    if (branch.Dynamic==true && branch.Termination==false)
    {
        bool StateChanged=false;
        updateState(branch, params,TransitionRates,StateVelocity,StateChanged); // Update state (e.g., growing, paused)
        if(!StateChanged){
        updateGrowth(branch, allBranches, Deletion_List, params,StateVelocity,BoxCenter,BoxWidth,soft_length); // Extend or modify branch tip  
        }
    }
        
    
}

