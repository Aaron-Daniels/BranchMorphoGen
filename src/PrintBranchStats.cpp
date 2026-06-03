#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <limits>  // for std::numeric_limits
#include "PrintBranchStats.h"
#include "Banner.h"

void PrintBranchStats::printBranchStats(std::vector<Branch>& allBranches, const SimulationParameters& params, const int sample, const double tsim) {
    std::ostringstream oss;
    oss << "BranchStats-" << sample << "-time-" << tsim;
    std::string filename = params.SimulationName + "-" + oss.str() + ".csv";

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write BranchStats.csv file");
    }

file << std::setprecision(4) << std::fixed; // control decimal precision
file << BranchMorphoGen::HASHBANNER;
file << "#Simulation name: " << params.SimulationName << ".\n";
file << "#file contains statistics of each branch.\n";
file << "#Info=All lengths are in "<<params.SpatialUnit<<" & "<<params.TemporalUnit<<std::endl;
file << "#Branch indices start from 0 "<<std::endl;
file << "#COLUMN_NAMES:"<<std::endl; 
file << "ID, Dynamic Type, State, Length, Radius, Parent ID, Child1 ID, Child2 ID" << std::endl;

for (auto& B : allBranches) {
    //Vec3 a = B.Points.back() - B.BasePoint;
    //double tortuosity = B.Length / a.norm();

    // Set `state` as string: either the state or "NaN"
    std::string state_str = B.Dynamic ? std::to_string(params.States[B.State]) : "NaN";
    std::string ch1_str = B.Dynamic ? "NaN" : std::to_string(B.Child1_ID) ;
    std::string ch2_str = B.Dynamic ? "NaN" : std::to_string(B.Child2_ID) ;

    file << B.ID << ", "
         << B.Dynamic << ", "
         << state_str << ", "
         << std::fixed << std::setprecision(3)
         << B.Length << ", "
         << B.Radius << ", "
         << B.Parent_ID << ", "
         << ch1_str << ", "
         << ch2_str << std::endl;
}
 file.close();



   
}



