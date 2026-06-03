/******************************************************************************
 * File:        Utilities.h
 * Project:     Branching Morphogenesis Simulator
 * Author:      Dr. Sabyasachi Sutradhar
 * Affiliation: Howard Lab, Yale University
 * Email:       sabyasachi.sutradhar@yale.edu
 *
 * Description:
 *   This module contains differernt utility functions,
 eg.    the Branch Struct, 
        BoundingBox struct, 
        some inpurt parser helper functions etc.
 * License:
 *   This software is developed for academic research purposes and is
 *   distributed under the MIT License (or specify another if applicable).
 *
 * Last Modified:    Jul 1, 2025
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

#ifndef UTILITIES_H
#define UTILITIES_H
#include <iostream>
#include <vector>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm> // for std::min_element
#include <stdexcept> // for std::invalid_argument
#include <limits> 
#include <functional>
#include <unordered_set>
#include <unordered_map>   // For std::unordered_map
#include <set>             // (if using std::set somewhere)
#include "RandomUtils.h"

constexpr double EPSILON = 1.0e-12;
inline double AdjustTol(double a) { return (fabs(a) < EPSILON) ? 0.0 : a; }

inline double diff(double a, double b)
{
    return (fabs(a - b) <= EPSILON ? 0.0 : (a - b));
}

inline int index_of_min(const std::vector<double> &v)
{
    if (v.empty())
    {
        throw std::invalid_argument("Cannot find minimum index of an empty vector.");
    }
    return std::distance(v.begin(), std::min_element(v.begin(), v.end()));
}

inline double WrapAngleTo2Pi(double angle)
{
    const double two_pi = 2.0 * M_PI;
    angle = fmod(angle, two_pi);
    if (angle < 0)
        angle += two_pi;
    return angle;
}



struct Vec3
{
    double x = 0.0, y = 0.0, z = 0.0;
     // Access components by index: 0 → x, 1 → y, 2 → z

    double& operator[](int i) {
        switch (i) {
            case 0:  return x;
            case 1:  return y;
            case 2:  return z;
            default: throw std::out_of_range("Vec3 index out of range");
        }
    }

    // Const version for read-only access
    const double& operator[](int i) const {
        switch (i) {
            case 0:  return x;
            case 1:  return y;
            case 2:  return z;
            default: throw std::out_of_range("Vec3 index out of range");
        }
    }

    Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }

    Vec3 operator+=(const Vec3 &o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }

    Vec3 operator-=(const Vec3 &o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }


    bool isFinite() const
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
    Vec3 addScalar(double s) const { return {x + s, y + s, z + s}; }

    double norm() const { return sqrt(x * x + y * y + z * z); }
    double invNorm() const { return 1.0 / sqrt(x * x + y * y + z * z); }
    double squaredNorm() const { return x * x + y * y + z * z; }

    void normalize()
    {
        double n = norm();
        if (n > 0)
        {
            x /= n;
            y /= n;
            z /= n;
        }
    }

    Vec3 normalized() const
    {
        double n = norm();
        if (n > 0)
            return *this / n;
        return Vec3{0.0, 0.0, 0.0};
    }
    double calculateDistance(const Vec3 &o) const
    {
        double dx = x - o.x, dy = y - o.y, dz = z - o.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }

    double calculateDistanceSqr(const Vec3 &o) const
    {
        double dx = x - o.x, dy = y - o.y, dz = z - o.z;
        return dx * dx + dy * dy + dz * dz;
    }

    Vec3 calculateDirectionCosine(const Vec3 &o) const
    {
        Vec3 dir = o - *this;
        double len = dir.norm();
        if (len == 0.0)
            return {0.0, 0.0, 0.0};
        return dir / len; // Normalize
    }

    Vec3 crossProduct(const Vec3 &b)
    {
        return {
            y * b.z - z * b.y,
            z * b.x - x * b.z,
            x * b.y - y * b.x};
    }

    bool isInsideVoxel(const Vec3 &o, double margin)
    {
        return (o.x > x - margin && o.x < x + margin) &&
               (o.y > y - margin && o.y < y + margin) &&
               (o.z > z - margin && o.z < z + margin);
    }

    double dotProduct(const Vec3 &o) { return x * o.x + y * o.y + z * o.z; }

    static double angle_between(Vec3 &u, Vec3 &v)
    {
        
        double dot_product = u.dotProduct(v);
        double cross_norm = (u.crossProduct(v)).norm();
        double angle = std::atan2(cross_norm, dot_product); // returns angle in radians

        return angle; // in radians, between 0 and π


    }

    static Vec3 randomUnitVector()
    {
        double theta = 2.0 * M_PI * RandomUtils::UnitUniform();
        double phi = std::acos(2.0 * RandomUtils::UnitUniform() - 1.0);
        double sin_phi = std::sin(phi);
        return {
            sin_phi * std::cos(theta),
            sin_phi * std::sin(theta),
            std::cos(phi)};
    }


    double squaredDistanceToSegment(const Vec3 &a, const Vec3 &b) const
    {
        Vec3 ab = b - a;
        Vec3 ap = *this - a;
        double ab2 = ab.squaredNorm();
        if (ab2 == 0.0)
            return calculateDistanceSqr(a); // segment is a point

        double t = ap.dotProduct(ab) / ab2;
        t = std::max(0.0, std::min(1.0, t)); // clamp to [0,1]
        Vec3 closest = a + ab * t;
        return calculateDistanceSqr(closest);
    }

 static Vec3 sampleLocalSpherical(double theta, double phi) {
    double x = sin(theta) * cos(phi);
    double y = sin(theta) * sin(phi);
    double z = cos(theta);
    return Vec3{x, y, z}; // In local Z-aligned frame
    }

static Vec3 rotateZTo(const Vec3& localDir, const Vec3& tangent) {
    Vec3 z = Vec3{0, 0, 1};
   

    if ((tangent - z).norm() < 1e-6) return localDir;
    if ((tangent + z).norm() < 1e-6) return Vec3{-localDir.x, -localDir.y, -localDir.z};

    Vec3 axis = z.crossProduct(tangent).normalized();
    double angle = acos(z.dotProduct(tangent));

    return localDir * cos(angle)
         + axis.crossProduct(localDir) * sin(angle)
         + axis * (axis.dotProduct(localDir)) * (1.0 - cos(angle));
}

};

struct BoundingBox
{
    Vec3 min{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()
    };
    Vec3 max{
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };

    void expand(double padding)
    {
        min.x -= padding;
        min.y -= padding;
        min.z -= padding;
        max.x += padding;
        max.y += padding;
        max.z += padding;
    }

    void expandToInclude(const Vec3 &pt)
    {
        min.x = std::min(min.x, pt.x);
        min.y = std::min(min.y, pt.y);
        min.z = std::min(min.z, pt.z);
        max.x = std::max(max.x, pt.x);
        max.y = std::max(max.y, pt.y);
        max.z = std::max(max.z, pt.z);
    }

     void expandToInclude(const BoundingBox &box) {
        expandToInclude(box.min);
        expandToInclude(box.max);
    }

    bool intersects(const BoundingBox &other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

bool isInside(const Vec3 &p, double margin) const
{
    auto lower = [&](double minv, double pv) {
        if (std::isinf(minv) && minv < 0) return true;
        return pv > (minv - margin);
    };

    auto upper = [&](double maxv, double pv) {
        if (std::isinf(maxv) && maxv > 0) return true;
        return pv < (maxv + margin);
    };

    return lower(min.x, p.x) && upper(max.x, p.x) &&
           lower(min.y, p.y) && upper(max.y, p.y) &&
           lower(min.z, p.z) && upper(max.z, p.z);
}

};
////// Help functions for Inpurt parser

// Utility functions (adjust if you have your own versions)
inline std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}
inline std::string toUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}
inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::string removeComments(const std::string& s) {
    size_t pos1 = s.find("//");
    size_t pos2 = s.find("#");
    size_t pos3 = s.find("%");
    size_t pos = std::min({ 
        pos1 == std::string::npos ? s.size() : pos1, 
        pos2 == std::string::npos ? s.size() : pos2,
        pos3 == std::string::npos ? s.size() : pos3 
    });
    return s.substr(0, pos);
}


inline std::vector<std::string> split(const std::string& s, char delimiter, bool ignoreEmpty = true) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream ss(s);
    while (std::getline(ss, token, delimiter)) {
        token = trim(token);
        if (ignoreEmpty && token.empty()) continue;
        tokens.push_back(token);
    }
    return tokens;
}

// Parsing primitive types
inline bool parseBool(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str == "true" || str == "1" || str=="yes") return true;
    if (str == "false" || str == "0" || str=="no") return false;
    throw std::runtime_error("Invalid boolean value: " + s);
}

// Parse Vec3: "x,y" or "x,y,z"
inline Vec3 parseVec3(const std::string& s) {
    auto parts = split(s, ',');
    if (parts.size() == 2) {
        return Vec3{std::stod(parts[0]), std::stod(parts[1]), 0.0};
    } else if (parts.size() == 3) {
        return Vec3{std::stod(parts[0]), std::stod(parts[1]), std::stod(parts[2])};
    } else {
        throw std::runtime_error("Invalid Vec3 format: " + s);
    }
}
// Parse pair<int,int>: "a, b"
inline std::pair<int, int> parsePairInt(const std::string& valueStr) {
    std::string cleaned;
    // Remove any whitespace
    for (char c : valueStr) {
        if (!std::isspace(static_cast<unsigned char>(c))) cleaned += c;
    }

    size_t dashPos = cleaned.find('-');
    if (dashPos != std::string::npos) {
        // It's a range: "2-6"
        int minVal = std::stoi(cleaned.substr(0, dashPos));
        int maxVal = std::stoi(cleaned.substr(dashPos + 1));
        return {minVal, maxVal};
    } else {
        // It's a single value: "5"
        int val = std::stoi(cleaned);
        return {val, val};
    }
}

// Parse vector<T> (comma separated)
template<typename T>
inline std::vector<T> parseVector(const std::string& s, const std::function<T(const std::string&)>& parseFunc) {
    std::vector<std::string> tokens = split(s, ',');
    std::vector<T> result;
    for (const auto& token : tokens) {
        result.push_back(parseFunc(token));
    }
    return result;
}
#endif // end of UTILITIES.h