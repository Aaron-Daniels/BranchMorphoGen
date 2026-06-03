/******************************************************************************
 * File:        SimulationParameters.h
 * Project:     Branching Morphogenesis Generator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This header file  sets up the input parameters
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Jul 31, 2025
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
#ifndef SIMULATIONPARAMETERS_H
#define SIMULATIONPARAMETERS_H
#include <cmath>
#include <string>
#include <vector>
#include "Utilities.h"
#include "InterpolatedParameter.h"


// Holds all configurable parameters for the simulation
struct SimulationParameters {
    std::string SpatialUnit;                 // Unit for spatial measurements (e.g., micron)
    std::string TemporalUnit;                // Unit for temporal measurements (e.g., second)
    std::string SimulationName;              // Name identifier for the simulation output
    int NSample;                             // Number of simulation samples to run
    bool RunParallel;                       // run Simulation parallely? true/false/yes/no/
    int Dimension;                           // Dimensionality of the simulation (2 or 3)
    double Dt;                               // Time step size
    double Time_Start;                       // Simulation end time
    double Time_End;                         // Simulation end time
    double PersistanceLength;                // Persistence length for directional memory in growth

    bool ReadFromSWC;                           // Read initial conf from .swc?
    std::string SWCFileName;                   // Filename of .swc file
    Vec3 Root;                               // Root position of the initial branch
    std::pair<int,int>N_BranchInitialRange;  // Range of initial branches at the root
    int N_BranchInitial;                     // Number of initial branches at the root
    double InitialAngle;                        // initial branches with respect to parent
    double InitialAngleWithZ;                // Angle of initial branches with the Z-axis
    double InitialLength;                    // Length of initial branches
    double R_Soma;                           // Radius of the soma (cell body)
    double NascentBranchLength;              // Length of newly spawned branches
    double BranchInterval;                   // Minimum time between branch events from a tip

    std::vector<int> States;                 // List of possible dynamic states for a tip
    InterpolatedParameter<std::vector<double>> StateVelocity;
    InterpolatedParameter<std::vector<std::vector<double>>> TransitionRates;

    bool Bifurcation;                        // Flag to enable bifurcation-type branching
    double BifurcationRatio;                   // Ratio between bifurcating angles
    bool FreezeCollidedBranch;                // if true collided branches do not spawn new branches
    std::string BranchingAngleRef;           // Branching angle reference frame: optins Global/local
    std::string PolarAngleMode;              // Options: fixed/uniform/reflective
    std::string AzimuthalAngleMode;          // options: fixed/uniform/reflective
    double PolarBranchingAngle;              // polar Angle between parent and daughter branches
    double AzimuthalBranchingAngle;          // azimuthal angle between parent and daughter branches
    double BranchingAngleSTD;                  /// std deviation of branching angle
    bool LengthDependentBranching;           // If true, branching probability depends on branch length
    InterpolatedParameter<double> BranchingRate; //Branching rate could be const, or dynamic
    double RebranchingProb=0.0;                  // Probability of re-branching from existing branches
    double TerminationRate=0.0;

    InterpolatedParameter<std::vector<double>> LimitsX;             // X-axis spatial boundaries [min, max]
    InterpolatedParameter<std::vector<double>> LimitsY;            // Y-axis spatial boundaries [min, max]
    InterpolatedParameter<std::vector<double>> LimitsZ;             // Z-axis spatial boundaries [min, max]

    double Boundary_SoftLength;                  /// measure the width of the soft boundary in length scale

    bool SelfAvoiding;                         // Are barches self-avoiding
    double Interaction_Threshold;            // Distance threshold for detecting interactions
    double PostCollisionPeriod;              // Time delay before a collided branch can grow again
    std::vector<int> Collision_States;       // Special states used during or after collisions
    std::vector<double> Collision_StateVelocity; // Velocities in the collision states
    std::vector<std::vector<double>> Collision_TransitionRates; // Transitions within collision states

    bool StraightenBranches;                 // If true, straighten branches via mechanical forces
    double E_Axial;                          // Spring constant for branch stretch
    double E_Bending;                         // Bending stiffness coefficient of branches
    double DragCoef;                              // Viscous drag coefficient

    std::string RadiusModel;                // choose radius model for branches
    double MinimumRadius;                   // minumum radius of branches
    double MaximumRadius;                   //maximum radius of branches
    double BetaRadius;                       // Controls thickness of branch with leaf no/subtree size/subtree length/and subtree volume etc
    
    int N_SWC;                               // Number of frames for which to output SWC files
    std::string ImagePlane;
    double MAX_IMAGE_SIZE;                   // Size (in spatial units) of generated images
    double pixelsize;                        // Size of one pixel in spatial units
    double SignalMean;                       // Mean signal intensity for image generation
    double SignalStd;                        // Standard deviation of signal intensity
    double BackgroundMean;                   // Mean background noise intensity
    double BackgroundStd;                    // Standard deviation of background noise

    ///////// Derived Parameters  /////////


    int NState;                              // Total number of tip states
    int Collision_NState;                    // Number of states used in collision context
    double PerisitanceInv;                   // Inverse of persistence length for efficiency
    double CollisionPadding;                 // Extra padding added to bounding box for interactions
    int CollisionSkip;                    // Frequency (in steps) of neighbor list updates
    std::vector<Vec3> BoundaryPoints;        // Corner points defining simulation space bounding box
    BoundingBox Boundary;                    // Bounding box for the simulation storing the spatial limits

    int ForceSkip;                           // Number of steps to skip between force computations

    double Drag;                             // Drag coefficient for segments
    double DragInv;                          // Precomputed inverse drag for speedup
    int SWC_frequency;                       // Frequency of SWC file output during simulation


    // ======= Time-dependent Updater =======
    void updateBoundary(double tsim) {
        std::vector<double> xlim = LimitsX(tsim);
        std::vector<double> ylim = LimitsY(tsim);
        std::vector<double> zlim = LimitsZ(tsim);

        if (xlim.size() == 2 && ylim.size() == 2 && zlim.size() == 2) {
            BoundaryPoints[0] = {xlim[0], ylim[0], zlim[0]};
            BoundaryPoints[1] = {xlim[1], ylim[1], zlim[1]};
            Boundary = BoundingBox();  // Reset
            Boundary.expandToInclude(BoundaryPoints[0]);
            Boundary.expandToInclude(BoundaryPoints[1]);
        }

    }

    // Optional: shorthand version
    void operator()(double tsim) {
        updateBoundary(tsim);
    }


};

// Parses input parameter file and returns a SimulationParameters object
SimulationParameters parseInputFile(const std::string& filename);

#endif // SIMULATIONPARAMETERS_H

