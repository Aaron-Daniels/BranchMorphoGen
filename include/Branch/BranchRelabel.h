#pragma once
#include "BranchCommon.h"

namespace BranchRelabel
{
    void relabelIndex(std::vector<Branch> &allBranches);

    void mergeTipsAfterDeletion(int branch_id,
                                std::vector<Branch> &allBranches,
                                std::vector<int> &Deletion_List,
                                const SimulationParameters &params);

    void debranching(std::vector<Branch> &allBranches, 
                    std::vector<int> &Deletion_List,
                     const SimulationParameters &params);
}
