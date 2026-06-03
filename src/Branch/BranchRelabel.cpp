/******************************************************************************
 * File:        BranchCollision.cpp
 * Project:     Branching Morphology Generator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *  This module relabels the brachnes after deletion
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Dec 12, 2025
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
#include "Branch/BranchRelabel.h"

inline void copyAndOverwritePrimaryNeighbors(Branch& dst, const Branch& src)
{
    dst.Neighbors = src.Neighbors;
    if (dst.Neighbors.size() < 3)
        dst.Neighbors.resize(3, -1);

    dst.Neighbors[0] = dst.ID;
    dst.Neighbors[1] = (dst.Parent_ID  >= 0) ? dst.Parent_ID  : -1;
    dst.Neighbors[2] = (dst.Sibling_ID >= 0) ? dst.Sibling_ID : -1;
}
    //////////////////////////////////////  Debranching ////////////////////////////

inline bool siblingInDeletionList(int siblingID, std::vector<int> &Deletion_List)
    {
        return std::find(Deletion_List.begin(), Deletion_List.end(), siblingID) != Deletion_List.end();
    }
    ///////////////////////////////////. Relabel indices //////////////////////
void  BranchRelabel::relabelIndex(std::vector<Branch> &allBranches)
    {
    const int oldN = static_cast<int>(allBranches.size());
    std::vector<int> NewBranchIndex(oldN, -1);
    std::vector<Branch> newBranches;
    newBranches.reserve(oldN);

    int k = 0;

    // First pass: mark surviving branches
    for (int i = 0; i < oldN; ++i) {
        if (!allBranches[i].Deletion) {
            NewBranchIndex[i] = k;
            ++k;
        }
    }

    // Second pass: remap indices and rebuild vector
    for (int i = 0; i < oldN; ++i) {
        if (allBranches[i].Deletion) {
            continue;
        }

        Branch &b = allBranches[i];

        int oldParent  = b.Parent_ID;
        int oldSibling = b.Sibling_ID;
        int oldChild1  = b.Child1_ID;
        int oldChild2  = b.Child2_ID;

        // New ID
        b.ID = NewBranchIndex[i];

    
             // Parent and sibling
        if (b.Initial) {
            b.Parent_ID  = -1;
            b.Sibling_ID = -1;
        } else {
            if (oldParent >= 0 && oldParent < oldN) {
                b.Parent_ID = NewBranchIndex[oldParent];
            } else {
                b.Parent_ID = -1;
            }

            if (oldSibling >= 0 && oldSibling < oldN) {
                b.Sibling_ID = NewBranchIndex[oldSibling];
            } else {
                b.Sibling_ID = -1;
            }
        }

        // Children
        if (!b.Dynamic) {
            if (oldChild1 >= 0 && oldChild1 < oldN) {
                b.Child1_ID = NewBranchIndex[oldChild1];
            } else {
                b.Child1_ID = -1;
            }

            if (oldChild2 >= 0 && oldChild2 < oldN) {
                b.Child2_ID = NewBranchIndex[oldChild2];
            } else {
                b.Child2_ID = -1;
            }
        } else {
            b.Child1_ID = -1;
            b.Child2_ID = -1;
        }
        ///////// fix the neighbors
        ///////// fix the neighbors
const auto oldneighbors = b.Neighbors;

if (b.Dynamic) {

    b.Neighbors.resize(oldneighbors.size(), -1);

    for (int kk = 0; kk < static_cast<int>(oldneighbors.size()); ++kk) {
        const int oldId = oldneighbors[kk];

        if (oldId >= 0 && oldId < oldN) {
            b.Neighbors[kk] = NewBranchIndex[oldId];   // can still become -1 if deleted
        } else {
            b.Neighbors[kk] = -1;
        }
    }

    //// make sure first three are connected branches
    if (b.Neighbors.size() < 3) b.Neighbors.resize(3, -1);

    b.Neighbors[0] = b.ID;
    b.Neighbors[1] = (b.Parent_ID  >= 0) ? b.Parent_ID  : -1;
    b.Neighbors[2] = (b.Sibling_ID >= 0) ? b.Sibling_ID : -1;

    //// compact and remove obvious duplicates
    int w = 3;
    for (int r = 3; r < static_cast<int>(b.Neighbors.size()); ++r) {
        const int v = b.Neighbors[r];
        if (v < 0) continue;
        if (v == b.Neighbors[0] || v == b.Neighbors[1] || v == b.Neighbors[2]) continue;
        b.Neighbors[w++] = v;
    }
    b.Neighbors.resize(static_cast<size_t>(w));

    } else {
    b.Neighbors.clear();
    }


   
        b.Deletion = false;
        b.LastDist = b.calculateLastDist();
        b.updateBoundingBox();
        newBranches.push_back(b);
        
    }

    allBranches = std::move(newBranches);

}
    ////////////////////// Merge tips after deletion ////////////////////////////

void BranchRelabel::mergeTipsAfterDeletion(int branch_id,
                            std::vector<Branch> &allBranches,
                            std::vector<int> &Deletion_List,
                            const SimulationParameters &params)
{
    const int N = static_cast<int>(allBranches.size());

    // Basic guard
    if (branch_id < 0 || branch_id >= N) {
        //std::cerr << "mergeTipsAfterDeletion: invalid branch_id "
          //        << branch_id << " with N = " << N << std::endl;
        return;
    }

    Branch &b = allBranches[branch_id];
    int parentID  = b.Parent_ID;
    int siblingID = b.Sibling_ID;   

    // Validate parent and sibling indices
    auto valid_index = [N](int id) {
        return id >= 0 && id < N;
    };

    if (!valid_index(parentID) || !valid_index(siblingID) || parentID == siblingID) {
        /*
                std::cerr << "mergeTipsAfterDeletion: invalid parent or sibling for branch "
                  << branch_id
                  << "  parentID = " << parentID
                  << "  siblingID = " << siblingID
                  << "  N = " << N << std::endl;
        */
        // At minimum mark this branch deleted to avoid reuse
        b.Deletion = true;
        return;
    }

    Branch &parent  = allBranches[parentID];
    Branch &sibling = allBranches[siblingID];

    // If sibling will also be deleted, do not splice its points into parent
    if (siblingInDeletionList(siblingID, Deletion_List)) {
        parent.Dynamic   = true;
        parent.Collision = false;
        parent.Deletion  = false;
        parent.State     = 0;
        std::vector<double> velocity = params.StateVelocity.getValue(0.0);
        parent.Velocity  = velocity[parent.State];
        parent.Child1_ID = -1;
        parent.Child2_ID = -1;

        parent.Neighbors.resize(3, -1);
        parent.Neighbors[0] = parent.ID;
        parent.Neighbors[1] = (parent.Parent_ID  >= 0) ? parent.Parent_ID  : -1;
        parent.Neighbors[2] = (parent.Sibling_ID >= 0) ? parent.Sibling_ID : -1;
        
    } else {
        // Splice sibling geometry into parent, but only if sibling has points
        if (!sibling.Points.empty()) {
            parent.Points.insert(parent.Points.end(),
                                 sibling.Points.begin(),
                                 sibling.Points.end());
        }

        parent.updateBoundingBox();
        parent.Length   = parent.calculateContourLength();
        parent.LastDist = parent.calculateLastDist();

        parent.Collision = sibling.Collision;
        parent.PostCollision_Time = sibling.PostCollision_Time;
        parent.Deletion  = false;

        parent.Dynamic   = sibling.Dynamic;
        parent.State     = sibling.State;
        parent.Velocity  = sibling.Velocity;
        parent.Termination = sibling.Termination;

        // Update children only if their indices are valid
        if (!sibling.Dynamic) {
            if (valid_index(sibling.Child1_ID)) {
                parent.Child1_ID = sibling.Child1_ID;
                allBranches[sibling.Child1_ID].Parent_ID = parentID;
            } else {
                parent.Child1_ID = -1;
            }

            if (valid_index(sibling.Child2_ID)) {
                parent.Child2_ID = sibling.Child2_ID;
                allBranches[sibling.Child2_ID].Parent_ID = parentID;
            } else {
                parent.Child2_ID = -1;
            }
        } else {
            parent.Child1_ID = -1;
            parent.Child2_ID = -1;
        }

        copyAndOverwritePrimaryNeighbors(parent, sibling);
    }

    // Finally mark the current branch and sibling as deleted
    b.Deletion       = true;
    sibling.Deletion = true;
}


void BranchRelabel::debranching(std::vector<Branch> &allBranches, std::vector<int> &Deletion_List, const SimulationParameters &params)
    {
        
        if (Deletion_List.size() > 0)
        {
            for (size_t id = 0; id < Deletion_List.size(); ++id)
            {
                int iid = Deletion_List[id];
                // cout<<" #######1.  Deletion Id "<< iid << " Parent ID "<<allBranches[iid].ParentID<<" Sibling ID "<<allBranches[iid].Sibling_ID;
                // cout<<" #######2.  Deletion "<< allBranches[iid].Deletion << " Parent ID "<<allBranches[allBranches[iid].ParentID].Deletion<<" Sibling ID "<<allBranches[allBranches[iid].Sibling_ID].Deletion<<endl;
                mergeTipsAfterDeletion(iid, allBranches, Deletion_List, params);
                // cout<<" #######3.  Deletion "<< allBranches[iid].Deletion << " Parent ID "<<allBranches[allBranches[iid].ParentID].Deletion<<" Sibling ID "<<allBranches[allBranches[iid].Sibling_ID].Deletion<<endl;
            }

            relabelIndex(allBranches);
        }
    }
