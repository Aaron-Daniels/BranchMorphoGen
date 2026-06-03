#pragma once
#include "BranchCommon.h"

    
namespace BranchGrowth
{
    void updateState(
                    Branch &branch, 
                    const SimulationParameters &params,
                    const std::vector<std::vector<double>> &TransitionRates,
                    const std::vector<double> &StateVelocity,
                    bool &StateChanged);

    void addLengthToTip(
                    Branch &branch,
                    std::vector<Branch> &allBranches, 
                    double ladd,
                    std::vector<int> &Deletion_List,
                    const SimulationParameters &params,
                    const Vec3 BoxCenter,
                    const Vec3 BoxWidth,
                    const double soft_length);

    void retractLengthFromTip(
                    std::vector<Branch> &allBranches,    
                    Branch &branch,
                    double length_to_remove,
                    std::vector<int> &Deletion_List, 
                    const SimulationParameters &params,
                    const std::vector<double> &StateVelocity);

    void updateGrowth(Branch &branch, 
                    std::vector<Branch> &allBranches,
                    std::vector<int> &Deletion_List,
                    const SimulationParameters &params,
                    const std::vector<double> &StateVelocity,
                    const Vec3 BoxCenter,
                    const Vec3 BoxWidth,
                    const double soft_length);

    void elogationDynamics(Branch &branch, 
                    std::vector<Branch> &allBranches,
                    std::vector<int> &Deletion_List,
                    const SimulationParameters &params,
                    const std::vector<std::vector<double>> &TransitionRates,
                    const std::vector<double> &StateVelocity,
                    const Vec3 BoxCenter,
                    const Vec3 BoxWidth,
                    const double soft_length);



}
