
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "SimulationParameters.h"

#include <filesystem>
namespace fs = std::filesystem;

std::vector<std::string> getSWCFilenames(SimulationParameters& params) {
    
    std::string inputPath=params.SWCFileName;   

    std::vector<std::string> filenames;
    
    if (inputPath.empty()) {
        throw std::runtime_error("ReadFromSWC is true but InputPath is empty.");
    }

    fs::path path(inputPath);

    if (fs::is_regular_file(path) && path.extension() == ".swc") {
        filenames.push_back(path.string());
    } else if (fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".swc") {
                filenames.push_back(entry.path().string());
            }
        }
    } else {
        throw std::runtime_error("Invalid input: must be a .swc file, a folder containing .swc files, or left empty.");
    }
    
    if (filenames.empty()) {
    throw std::runtime_error("No .swc files found in the specified path: " + inputPath);
    }

    params.NSample = filenames.size();
    
    return filenames;

}