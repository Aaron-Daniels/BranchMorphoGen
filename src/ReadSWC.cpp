#include "ReadSWC.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <stdexcept>
#include <iomanip>

namespace SWCParser {

struct SWCNode {
    int id, type, parentId;
    double x, y, z, radius;
};

// Local context struct to avoid shared globals
struct ParseContext {
    std::vector<SWCNode> nodes;
    std::vector<int> label;
    std::vector<int> labelbr;
    std::vector<std::vector<int>> ChildPtID;
    std::vector<Branch> branches;
    int NBranch = -1;
};

void make_subtree(int np, SimulationParameters& params, bool initial, ParseContext& ctx) {
    Branch branch;
    ctx.NBranch++;
    branch.Age = 0.0;
    branch.State = 0;
    branch.ID = ctx.NBranch;
    branch.Parent_ID = -1;
    branch.Child1_ID = -1;
    branch.Child2_ID = -1;
    branch.Sibling_ID = -1;
    branch.Velocity = 0.0;
    branch.Dynamic = false;
    branch.Initial = initial;
    branch.Collision = false;
    branch.PostCollision_Time = 0.0;
    branch.Deletion = false;
    branch.Length = 0.0;
    branch.LastDist=0.0;

    do {
        ctx.labelbr[np] = ctx.NBranch;
        Vec3 point = { ctx.nodes[np].x, ctx.nodes[np].y, ctx.nodes[np].z };
        branch.Points.push_back(point);
        np++;
    } while (ctx.label[np] == 0);

    ctx.labelbr[np] = ctx.NBranch;
    branch.Points.push_back(Vec3{ ctx.nodes[np].x, ctx.nodes[np].y, ctx.nodes[np].z });
    branch.Radius = ctx.nodes[np].radius;

    if (ctx.label[np] == 1) {
        branch.State = params.States[0];
        std::vector<double> velocity = params.StateVelocity.getValue(params.Time_Start);
        branch.Velocity = velocity[branch.State];
        branch.Dynamic = true;
    }

    ctx.branches.push_back(branch);

    if (ctx.label[np] == 2) {
        make_subtree(ctx.ChildPtID[np][0], params, false, ctx);
        make_subtree(ctx.ChildPtID[np][1], params, false, ctx);
    }
}

std::vector<Branch> SWCTreeParser::parse(const std::string& filename, SimulationParameters& params) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file " + filename);

    ParseContext ctx;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        SWCNode n;
        if (!(iss >> n.id >> n.type >> n.x >> n.y >> n.z >> n.radius >> n.parentId)) {
            std::cerr << "Malformed line: " << line << "\n";
            continue;
        }
        ctx.nodes.push_back(n);
    }

    Vec3 root = { ctx.nodes.front().x, ctx.nodes.front().y, ctx.nodes.front().z };
    params.Root = root;

    int NPoints = static_cast<int>(ctx.nodes.size());
    std::vector<int> InitialInd;
    InitialInd.clear();

    ctx.ChildPtID.clear();
    ctx.label.clear();
    ctx.labelbr.clear();

    ctx.ChildPtID.resize(NPoints, std::vector<int>(2, 0));
    ctx.label.resize(NPoints, 0);
    ctx.labelbr.resize(NPoints, 0);

    for (int i = 0; i < NPoints; ++i) {
        if (ctx.nodes[i].id - ctx.nodes[i].parentId != 1 && ctx.nodes[i].parentId != 1 && ctx.nodes[i].parentId != -1) {
            ctx.label[ctx.nodes[i].parentId - 1] = 2;
        }
        if (ctx.nodes[i].parentId == 1) {
            InitialInd.push_back(i);
        }
    }

    //std::cout << "Read .swc file " << filename << " successfully" << std::endl;

    for (int i = 0; i < NPoints; ++i) {
        bool flag = false;
        for (int j = 0; j < NPoints; ++j) {
            if (ctx.nodes[i].id == ctx.nodes[j].parentId) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            ctx.label[ctx.nodes[i].id - 1] = 1;
        }
    }

    for (int i = 0; i < NPoints; ++i) {
        if (ctx.label[i] == 2) {
            int nchild = 0;
            for (int j = 0; j < NPoints; ++j) {
                if (ctx.nodes[i].id == ctx.nodes[j].parentId) {
                    ctx.ChildPtID[i][nchild] = ctx.nodes[j].id - 1;
                    nchild++;
                }
            }
        }
    }

    ctx.NBranch = -1;
    params.N_BranchInitial = static_cast<int>(InitialInd.size());
    ctx.branches.clear();

    for (int idx : InitialInd) {
        make_subtree(idx, params, true, ctx);
    }

    for (int i = 0; i < NPoints; ++i) {
        if (ctx.label[i] == 2) {
            int pr = ctx.labelbr[i];
            int ch1 = ctx.labelbr[ctx.ChildPtID[i][0]];
            int ch2 = ctx.labelbr[ctx.ChildPtID[i][1]];
            ctx.branches[ch1].Parent_ID = pr;
            ctx.branches[pr].Child1_ID = ch1;
            ctx.branches[ch2].Parent_ID = pr;
            ctx.branches[pr].Child2_ID = ch2;
            ctx.branches[ch1].Sibling_ID = ch2;
            ctx.branches[ch2].Sibling_ID = ch1;
        }
    }

    for (auto& b : ctx.branches) {
        if (b.Initial) {
            b.BasePoint = root;
        } else {
            b.BasePoint = ctx.branches[b.Parent_ID].Points.back();
        }

        b.Length = b.calculateContourLength();
        b.updateBoundingBox();
        b.LastDist = b.calculateLastDist();
    }

    //std::cout << "Parsed everything successfully" << std::endl;
    return ctx.branches;
}

} // namespace SWCParser
