#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "PrintTime.h"


void PrintTimeFile::printTimeFile(std::vector<Branch>& allBranches,const std::string& filename, const double& tsim) {

    double Total_Length=0.0;
    size_t N_Tips=0;
    size_t NTotal=allBranches.size();

    for(auto & B:allBranches){
        if(B.Dynamic){N_Tips++;}
        Total_Length+=B.Length;
    }

  std::ofstream file(filename, std::ios::app);  //append lines

    if (!file.is_open()) {
        throw std::runtime_error("Cannot write Temporal properties file");
    }
    file << std::setprecision(4) << std::fixed; // control decimal precision
    file << tsim << ", "
         << NTotal << ", "
         << N_Tips << ", "
         <<Total_Length << ", "
         <<Total_Length/static_cast<double>(NTotal) << std::endl;

    file.close();

}

// namespace PrintTimeFile

