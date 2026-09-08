/******************************************************************************
 * File:        SimulationParameters.cpp
 * Project:     Branching Morphogenesis Generator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *  This module parses input parameters from an input file
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License.
 *
 * Last Modified:    Dec 09, 2025
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

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cctype>
#include <iomanip>

#include "Utilities.h"
#include "RandomUtils.h"
#include "SimulationParameters.h"



// Helper: parse key=value; pairs
inline bool parseKeyValue(const std::string& line, std::string& key, std::string& value) {
    auto pos = line.find('=');
    if (pos == std::string::npos) return false;
    key = trim(line.substr(0, pos));
    value = trim(line.substr(pos + 1));
    if (!value.empty() && value.back() == ';') {
        value.pop_back();
        value = trim(value);
    }
     // Sanity check: ensure key has no spaces
    if (key.find(' ') != std::string::npos) {
        std::cerr << "[WARNING] Key contains whitespace: '" << key << "'" << std::endl;
    }
    return !key.empty();
}

// Parse matrix: rows separated by ';', columns by ','
template<typename T>
std::vector<std::vector<T>> parseMatrix(const std::string& str, std::function<T(const std::string&)> parseElement) {
    std::vector<std::vector<T>> matrix;
    auto rows = split(str, ';');
    for (auto& rowStr : rows) {
        auto cleanRow = trim(rowStr);
        if (cleanRow.empty()) continue;
        auto values = split(cleanRow, ',');
        std::vector<T> row;
        for (auto& val : values) {
            row.push_back(parseElement(trim(val)));
        }
        matrix.push_back(row);
    }
    return matrix;
}

// Parse time-dependent/interpolated parameter block from a stringstream
template<typename T>
void parseInterpolatedParameter(std::istream& input,
                                InterpolatedParameter<T>& param,
                                double time_max,
                                const std::function<T(const std::string&)>& parseValue) {
    std::string line;
    bool foundTimeBased = false;

    while (std::getline(input, line)) {
        auto clean = trim(removeComments(line));
        if (clean.empty()) continue;

        //std::cout << "[DEBUG] Parsing line: '" << clean << "'\n";

        if (clean.find(':') != std::string::npos) {
            foundTimeBased = true;
            auto parts = split(clean, ':');
            if (parts.size() != 2) throw std::runtime_error("Invalid time:value format.");

            std::string timeStr = trim(parts[0]);
            std::string valueStr = trim(parts[1]);

            double time;
            if (toLower(timeStr) == "inf")
                time = time_max;
            else if (toLower(timeStr) == "-inf")
                time = -time_max;
            else
                time = std::stod(timeStr);

            T value = parseValue(valueStr);
            param.setTimeValue(time, value);

            //std::cout << "[DEBUG] Set timeValue[" << time << "]\n";
        } else {
            if (!foundTimeBased) {
                T value = parseValue(clean);
                param.setTimeValue(0.0, value);
                //std::cout << "[DEBUG] Set constant timeValue[0.0]\n";
                break;
            } else {
                throw std::runtime_error("Mixed time-based and constant values in interpolated parameter.");
            }
        }
    }

    if (param.timeValues.empty()) {
        //std::cout<<"This line throws error "<<std::endl; 
        //std::cout<<line<<std::endl; 
        std::string str= line + "No values defined for interpolation.";
        throw std::runtime_error(str);
    }

}


// Helper to parse multi-line interpolated vector block (e.g., XLimits, YLimits)
template<typename T>
bool parseInterpolatedVectorBlock(std::istream& file, const std::string& val, InterpolatedParameter<std::vector<T>>& param,
                                  double time_max, const std::string& tag, std::string& nextBufferedLine, bool& hasBufferedLine) {
    std::stringstream ss;
    ss << val << "\n";
    std::string line;
    while (std::getline(file, line)) {
        std::string cleanLine = trim(removeComments(line));
        if (cleanLine.empty()) continue;
        if (cleanLine.find('=') != std::string::npos) {
            nextBufferedLine = line;
            hasBufferedLine = true;
            break;
        }
        ss << cleanLine << "\n";
    }

    parseInterpolatedParameter<std::vector<T>>(ss, param, time_max,
        [](const std::string& s) {
            return parseVector<T>(s, [](const std::string& x) {
                return static_cast<T>(std::stod(trim(x)));
            });
        });

    return true;
}


// Parsing TransitionRates block (special case with buffering)
void parseTransitionRates(std::ifstream& file, const std::string& val, double time_max,
                          InterpolatedParameter<std::vector<std::vector<double>>>& param) {
    std::stringstream ss;
    ss << val << "\n";
    std::string line;
    std::streampos lastPos = file.tellg();

    while (std::getline(file, line)) {
        std::string cleanLine = trim(removeComments(line));
        if (cleanLine.empty()) continue;
        if (cleanLine.find('=') != std::string::npos) {
            file.seekg(lastPos);
            break;
        }
        ss << cleanLine << "\n";
        lastPos = file.tellg();
    }

    parseInterpolatedParameter<std::vector<std::vector<double>>>(
        ss, param, time_max,
        [](const std::string& s) {
            return parseMatrix<double>(s, [](const std::string& x) {
                return std::stod(trim(x));
            });
        });

}

// Main parser for input file
SimulationParameters parseInputFile(const std::string& filename) {
    SimulationParameters params;

    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open file: " + filename);

    std::string line;
    std::string bufferedLine;
    bool hasBufferedLine = false;

    while (true) {
        if (hasBufferedLine) {
            line = bufferedLine;
            hasBufferedLine = false;
        } else {
            if (!std::getline(file, line)) break;
        }



        line = trim(removeComments(line));
        if (line.empty()) continue;


//std::cout << "[PARSED LINE] " << line << std::endl;

std::string key, val;
if (!parseKeyValue(line, key, val)) {
   // std::cout << "[WARN] Could not parse line: " << line << std::endl;
    continue;
}

key = trim(key);  // <== add this
val = trim(val);
//std::cout << "[KEY] " << key << " | [VAL] " << val << std::endl;



        // Now parse based on key
        if (key == "SimulationName") {
            params.SimulationName = val;
        }
        else if (key == "SpatialUnit") {
            params.SpatialUnit = val;
        }
        else if (key == "TemporalUnit") {
            params.TemporalUnit = val;
        }
        else if (key == "NSample") {
            params.NSample = std::stoi(val);
        }
        else if(key == "RunParallel"){
            params.RunParallel=parseBool(val);
        }
        else if(key == "RandomSeed"){
            params.RandomSeed=static_cast<std::uint32_t>(std::stoul(val));
        }
        else if (key == "Dimension") {
            params.Dimension = std::stoi(val);
        }
        else if (key == "Dt") {
            params.Dt = std::stod(val);
        }
        else if(key == "Time_Start"){
            params.Time_Start=std::stod(val); 
        }
        else if (key == "Time_End") {
            params.Time_End = std::stod(val);
        }
        else if (key == "PersistanceLength") {
            if (val == "INF" || val == "inf")
                params.PersistanceLength = std::numeric_limits<double>::infinity();
            else
                params.PersistanceLength = std::stod(val);
        }       
        else if (key == "ReadFromSWC") {
            params.ReadFromSWC = parseBool(val);
        }
        else if (key == "SWCFileName") {
            params.SWCFileName = val;
        }

        else if (key == "Root") {
            params.Root = parseVec3(val);
        }
        else if (key == "N_BranchInitial") {
            params.N_BranchInitialRange = parsePairInt(val);
        }
        else if (key == "InitialAngle") {
            params.InitialAngle = std::stod(val);
        }
        else if (key == "InitialAngleWithZ") {
            params.InitialAngleWithZ = std::stod(val);
        }
        else if (key == "InitialLength") {
            params.InitialLength = std::stod(val);
        }
        else if (key == "RadiusSoma") {
            params.R_Soma = std::stod(val);
        }
        else if (key == "NascentBranchLength") {
            params.NascentBranchLength = std::stod(val);
        }
        else if (key == "BranchInterval") {
            params.BranchInterval = std::stod(val);
        }
        else if (key == "States") {
            params.States = parseVector<int>(val, [](const std::string& s) { return std::stoi(s); });
        }
        else if (key == "StateVelocity") {
            std::stringstream ss;
            ss << val << "\n";

            std::string nextLine;
            while (true) {
                if (hasBufferedLine) {
                    nextLine = bufferedLine;
                    hasBufferedLine = false;
                } else {
                    if (!std::getline(file, nextLine)) break;
                }
                std::string cleanNext = trim(removeComments(nextLine));
                if (cleanNext.empty()) continue;
                if (cleanNext.find('=') != std::string::npos) {
                    bufferedLine = nextLine;
                    hasBufferedLine = true;
                    break;
                }
                ss << cleanNext << "\n";
            }

            parseInterpolatedParameter<std::vector<double>>(ss, params.StateVelocity, params.Time_End,
                [](const std::string& s) {
                    return parseVector<double>(s, [](const std::string& x) { return std::stod(x); });
                });
        }
        else if (key == "TransitionRates") {
           // std::cout << "Parsing TransitionRates...\n";
            parseTransitionRates(file, val, params.Time_End, params.TransitionRates);
        }
        else if (key == "Bifurcation") {
            params.Bifurcation = parseBool(val);
        }
         else if (key == "BifurcationRatio"){
             params.BifurcationRatio = std::stod(val);
         }

        else if (key == "BranchingAngleRef") {
            params.BranchingAngleRef = toLower(val);
        }
        else if (key == "PolarAngleMode") {
            params.PolarAngleMode = toLower(val);
        }
        else if (key == "AzimuthalAngleMode") {
            params.AzimuthalAngleMode = toLower(val);
        }
        else if (key == "PolarBranchingAngle") {
            params.PolarBranchingAngle = std::stod(val);
        }
        else if (key == "AzimuthalBranchingAngle") {
            params.AzimuthalBranchingAngle = std::stod(val);
        }
        else if (key == "BranchingAngleSTD") {
            params.BranchingAngleSTD = std::stod(val);
        }

        else if (key == "LengthDependentBranching") {
            params.LengthDependentBranching = parseBool(val);
        }
        else if (key == "BranchingRate") {
            std::stringstream ss;
            ss << val << "\n";

            std::string nextLine;
            while (true) {
                if (hasBufferedLine) {
                    nextLine = bufferedLine;
                    hasBufferedLine = false;
                } else {
                    if (!std::getline(file, nextLine)) break;
                }
                std::string cleanNext = trim(removeComments(nextLine));
                if (cleanNext.empty()) continue;
                if (cleanNext.find('=') != std::string::npos) {
                    bufferedLine = nextLine;
                    hasBufferedLine = true;
                    break;
                }
                ss << cleanNext << "\n";
            }

            parseInterpolatedParameter<double>(ss, params.BranchingRate, params.Time_End,
                [](const std::string& s) { return std::stod(trim(s)); });
        }
        else if (key == "RebranchingProb") {
            params.RebranchingProb = std::stod(val);
        }
        else if (key == "TerminationRate") {
            params.TerminationRate = std::stod(val);
        }
        else if (key == "Boundary_SoftLength") {
            params.Boundary_SoftLength = std::stod(val);
        }

        else if (key == "LimitsX") {
            //std::cout<<"LimitsX in"<<std::endl;
           parseInterpolatedVectorBlock<double>(file, val, params.LimitsX, params.Time_End, "LimitsX", bufferedLine, hasBufferedLine);
            //std::cout<<"LimitsX done"<<std::endl;
        }
    else if (key == "LimitsY") {
    //std::cout<<"LimitsY in"<<std::endl;
    parseInterpolatedVectorBlock<double>(file, val, params.LimitsY, params.Time_End, "LimitsY", bufferedLine, hasBufferedLine);
    //std::cout<<"LimitsY done"<<std::endl;
    }
    else if (key == "LimitsZ") {
    //std::cout<<"LimitsZ in"<<std::endl;
    parseInterpolatedVectorBlock<double>(file, val, params.LimitsZ, params.Time_End, "LimitsZ", bufferedLine, hasBufferedLine);
    //std::cout<<"LimitsZ done"<<std::endl;
    }

        else if (key == "SelfAvoiding") {
            params.SelfAvoiding = parseBool(val);
        }
        else if (key == "FreezeCollidedBranch") {
            params.FreezeCollidedBranch = parseBool(val);
        }

        else if (key == "Interaction_Threshold") {
            params.Interaction_Threshold = std::stod(val);
        }
        else if (key == "PostCollisionPeriod") {
            if (val == "INF" || val == "inf")
                params.PostCollisionPeriod = std::numeric_limits<double>::infinity();
            else
                params.PostCollisionPeriod = std::stod(val);
        }
        else if (key == "Collision_States") {
            params.Collision_States = parseVector<int>(val, [](const std::string& s) { return std::stoi(s); });
        }
        else if (key == "Collision_StateVelocity") {
            params.Collision_StateVelocity = parseVector<double>(val, [](const std::string& s) { return std::stod(s); });
        }
        else if (key == "Collision_TransitionRates") {
            params.Collision_TransitionRates = parseMatrix<double>(val, [](const std::string& s) { return std::stod(s); });
        }
        else if (key == "StraightenBranches") {
            params.StraightenBranches = parseBool(val);
        }
        else if (key == "E_Axial") {
            params.E_Axial = std::stod(val);
        }
        else if (key == "E_Bending") {
            params.E_Bending = std::stod(val);
        }
        else if (key == "DragCoef") {
            params.DragCoef = std::stod(val);
        }
        else if (key == "RadiusModel") {
            params.RadiusModel = toLower(val);
        }
        else if (key == "MinimumRadius") {
            params.MinimumRadius = std::stod(val);
        }
        else if (key == "MaximumRadius") {
            params.MaximumRadius = std::stod(val);
        }
        else if (key == "BetaRadius") {
            params.BetaRadius = std::stod(val);
        }
        else if (key == "N_SWC") {
            params.N_SWC = std::stoi(val);
        }
        else if (key == "DumpKeypoints") {
            params.DumpKeypoints = parseBool(val);
        }
        else if (key == "ImagePlane") {
            params.ImagePlane = toLower(val);
        }
        else if (key == "MAX_IMAGE_SIZE") {
            params.MAX_IMAGE_SIZE = std::stod(val);
        }
        else if (key == "pixelsize") {
            params.pixelsize = std::stod(val);
        }
        else if (key == "SignalMean") {
            params.SignalMean = std::stod(val);
        }
        else if (key == "SignalStd") {
            params.SignalStd = std::stod(val);
        }
        else if (key == "BackgroundMean") {
            params.BackgroundMean = std::stod(val);
        }
        else if (key == "BackgroundStd") {
            params.BackgroundStd = std::stod(val);
        }
        else {
            std::cerr << "Warning: Unknown key '" << key << "' in input file.\n";
        }
    }
 

    ////////////////// Convert angle to radians
    const double deg2rad=M_PI/180.0;
    double angle=(params.InitialAngle<0.0)? RandomUtils::Uniform(0.0,2.0*M_PI):params.InitialAngle*deg2rad;
    params.InitialAngle=angle;
    params.InitialAngleWithZ=params.InitialAngleWithZ*deg2rad;
    params.PolarBranchingAngle=params.PolarBranchingAngle*deg2rad;
    params.AzimuthalBranchingAngle =params.AzimuthalBranchingAngle*deg2rad;
    params.BranchingAngleSTD=params.BranchingAngleSTD*deg2rad;

    ///////// Derived Parameters for Collision Skipping and Padding /////////
    params.NState = params.States.size();  // Total number of states
    params.PerisitanceInv = 1.0 / params.PersistanceLength;  // Precomputed for efficiency

    params.CollisionPadding = params.Interaction_Threshold + params.MinimumRadius;  // Inflates the bounding box for neighbor list search
    
    double maxAbsVelocity = 0.0;
   
    for (double v : params.StateVelocity.getValue(params.Time_Start)) {
        maxAbsVelocity = std::max(maxAbsVelocity, std::abs(v));
    }

    double skipval = params.CollisionPadding / (maxAbsVelocity*params.Dt);  // Number of steps to skip based on max velocity and padding

    int Skip = std::max(5,static_cast<int>(std::ceil(skipval / 5.0)) * 5);  // Round to nearest 5 for stability
    params.CollisionSkip=(Skip>10)? 10:Skip;
   //std::cout <<"Collision pad: "<<params.CollisionPadding<<" Skip value: "<<skipval<<", Collision Skip: "<<params.CollisionSkip<<std::endl;
    
    
    ////////// Setup Boundary Points and Bounding Box /////////
    params.BoundaryPoints.resize(2);
    auto xlim = params.LimitsX.getValue(params.Time_Start);
    auto ylim = params.LimitsY.getValue(params.Time_Start);
    auto zlim = params.LimitsZ.getValue(params.Time_Start);

    params.BoundaryPoints[0] = { xlim[0], ylim[0], zlim[0] };
    params.BoundaryPoints[1] = { xlim[1], ylim[1], zlim[1] };
    for (const auto& pt : params.BoundaryPoints) {
        params.Boundary.expandToInclude(pt);   // Final bounding box setup
    }
    // Bounding box object representing simulation limits

    ///////// Precompute Forece and Drag related parameters /////////
    params.ForceSkip = std::max(5,static_cast<int>(1.0 / params.Dt));  // Compute forces every nth step (optimization)
    params.Collision_NState = params.Collision_States.size();  // Number of such states
    params.Drag = 4.0 * M_PI * params.BranchInterval * params.DragCoef;  // Stokes drag on a segment
    params.DragInv = 1.0 / params.Drag;  // Precomputed for performance

//////////// Compute SWC output frequency ////////////
    if (params.N_SWC > 1) {
        params.SWC_frequency = static_cast<int>(std::round((params.Time_End-params.Time_Start) / (params.Dt * static_cast<double>(params.N_SWC-1))));
    }
    else {
        params.SWC_frequency = static_cast<int>(std::ceil((params.Time_End-params.Time_Start) / params.Dt));
    }
    //////// in 2D image plane is always xy
    if(params.Dimension==2) params.ImagePlane="xy";

    return params;
}
