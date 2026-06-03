#include <cmath>
#include "Utilities.h"
#include "Branch/BranchCommon.h"
namespace PrintSWC
{
void connectPoints(const std::string& filename,
                   std::vector<Branch>& allBranches,
                   int branchIndex,
                   std::vector<int>& parentSWC_ID,
                   int& NTotalPoints);

 void printSWCFiles(std::vector<Branch>& allBranches, const SimulationParameters& params, const int sample, const double& tsim);
};

