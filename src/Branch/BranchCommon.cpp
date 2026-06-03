/******************************************************************************
 * File:        BranchCommon.cpp
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 This file is to make sure there is no circular dependencies. and 
 executes only the basic Branch constructors.
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License
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

#include "Utilities.h"
#include "RandomUtils.h"
#include "SimulationParameters.h"
#include "Branch/BranchCommon.h"

// Increments the age of the branch and tracks time since collision; resets collision after timeout
void Branch::updateAge(const SimulationParameters &params,const double growth_velocity)
{
    Age += params.Dt;
    
    if ( Collision ) {
    if (PostCollision_Time < params.PostCollisionPeriod){
        PostCollision_Time += params.Dt;
    }else{
        Collision = false;
        PostCollision_Time = 0.0;
        State = 0; // Reset to default state after collision period
        Velocity = growth_velocity;
    }
    }
}

// Computes the axis-aligned bounding box from the list of Points
void Branch::updateBoundingBox()
{
    if (Points.empty())
        return;
    Bbox.min = Bbox.max = Points[0]; // Initialize bounding box to first point
    for (const auto &pt : Points)
    {
        Bbox.expandToInclude(pt); // Expand bounding box to include each point
    }
}

// Calculates the full contour length from BasePoint through all intermediate Points
double Branch::calculateContourLength() const
{
    double length = 0.0;
    if (!Points.empty())
    {
        length += Points[0].calculateDistance(BasePoint); // From base to first point
        for (size_t k = 0; k + 1 < Points.size(); ++k)
        {
            length += Points[k].calculateDistance(Points[k + 1]); // Sum segment lengths
        }
    }
    return length;
}



double Branch::calculateLastDist() const
{
    double length = 0.0;
    size_t n=Points.size();
    if (n>=2){
    length=(Points[n-1]-Points[n-2]).norm();
    }
return length;
}


void Branch::updateTermination(const SimulationParameters &params){
if(!Termination && !Initial && params.TerminationRate>EPSILON){
   double prob=1.0-exp(-params.TerminationRate*params.Dt);
   if (RandomUtils::UnitUniform() < prob){
    Termination=true;
   }
}



}
