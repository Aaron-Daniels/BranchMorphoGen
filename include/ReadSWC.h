#pragma once

#include <vector>
#include <string>
#include "Branch/BranchCommon.h"

namespace SWCParser {
    class SWCTreeParser {
    public:
        static std::vector<Branch> parse(const std::string& filename,SimulationParameters& params);
    };
}
