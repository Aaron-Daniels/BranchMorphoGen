/******************************************************************************
 * File:        main.cpp
 * Project:     Branching Morphogenesis Generator (BranchMorphoGen)
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This is the main function to simulate branching morphogenesis.
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Dec 09, 2025
 *
 * Usage Notes:
 *   - All units are in microns and minutes unless otherwise specified.
 *   - Requires C++17 or later.
 *   - Dependencies: This codebase needs CMake, libtiff
 *
 * References:
 *   [1] Shree, Sutradhar, et al., Sci. Adv. 8, eabn0080 (2022) 29 June 2022
 *   [2] Ouyang, Sutradhar et al.  Nature Comm. 
 *
 ******************************************************************************/
#include <thread>
#include <vector>
#include <iostream>
#include <random>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <chrono>
#include <mutex>

#include "Banner.h"
#include "RandomUtils.h"
#include "SimulationParameters.h"
#include "InputSWCFile.h"
#include "Simulation.h"

// global mutex for thread-safe logfile writing
std::mutex logMutex;
std::string LOG_FILENAME;


// thread-safe logger (file already created/cleared once at start)
void logRuntime(int sampleIndex,
                std::chrono::steady_clock::time_point t0,
                std::chrono::steady_clock::time_point t1)
{
    using namespace std::chrono;

    long long secs = duration_cast<seconds>(t1 - t0).count();
 
    std::lock_guard<std::mutex> lock(logMutex);

    std::ofstream log(LOG_FILENAME, std::ios::app);   // safe append
    if (log) {
        log << "Sample " << sampleIndex
            << " finished in "
            << std::setw(2) << std::setfill('0') << secs << " seconds\n";
    }
}

int main(int argc, char* argv[]) {

    std::cout << BranchMorphoGen::BANNER;

if (argc > 1 && std::string(argv[1]) == "--about") {
    std::cout << "Version: " << BranchMorphoGen::VERSION << "\n";
    std::cout << "Last Build: " << BranchMorphoGen::BUILD << "\n";
    return 0;
}

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " input_file.in\n";
        return 1;
    }

    std::string inputFilename = argv[1];
    SimulationParameters params;

    try {
        std::cout << "Parsing input file: " << inputFilename << std::endl;
        params = parseInputFile(inputFilename);
        std::cout << "Parsed input file successfully. Now starting simulation..." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error reading input file: " << e.what() << std::endl;
        return 1;
    }

    // build the log filename
    LOG_FILENAME = params.SimulationName + "-RunTime.log";

    // create/overwrite the log file once
    {
        std::ofstream clearFile(LOG_FILENAME, std::ios::out);
        if (clearFile) {
            clearFile <<BranchMorphoGen::BANNER
                    << "Simulation name: "  << params.SimulationName << ".\n";
            clearFile << "-----------------------------------------\n\n";
        }    

    }
//////////// if reading from SWC, get the list of files ///////////////////////
    std::vector<std::string> swcFiles;
    if(params.ReadFromSWC){
    swcFiles = getSWCFilenames(params);
    }else{
    swcFiles.resize(params.NSample, ""); // empty strings for no SWC
    }
//////////////////////////////////////////////////////////////////////////////    
    // serial mode (or when parallel disabled)
    if (params.NSample == 1 || !params.RunParallel) {

        for (int i = 0; i < params.NSample; ++i) {

            auto t0 = std::chrono::steady_clock::now();
    
            RandomUtils::InitializeGenerator(params.RandomSeed + static_cast<unsigned int>(i));
            Simulation sim(swcFiles[i], params);
            sim.Run(params, i + 1);

            auto t1 = std::chrono::steady_clock::now();
            logRuntime(i + 1, t0, t1);
        }

    } else if (params.NSample > 1 && params.RunParallel) {

        std::vector<std::thread> threads;

        for (int i = 0; i < params.NSample; ++i) {

            threads.emplace_back([=]() mutable {

                auto t0 = std::chrono::steady_clock::now();

                SimulationParameters threadParams = params;
                RandomUtils::InitializeGenerator(threadParams.RandomSeed + static_cast<unsigned int>(i));

                Simulation sim(swcFiles[i], threadParams);
                sim.Run(threadParams, i + 1);

                auto t1 = std::chrono::steady_clock::now();
                logRuntime(i + 1, t0, t1);
            });
        }

        for (auto& t : threads) t.join();
    }

std::cout << "Simulation completed." << std::endl;

    return 0;
}
