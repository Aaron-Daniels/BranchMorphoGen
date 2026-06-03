#pragma once

#include <vector>
#include <map>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <cmath>
#include "Utilities.h"

// ======= Interpolation Functions =======

// Scalar
template<typename T>
T interpolateLinear(const T& a, const T& b, double alpha) {
    return a + alpha * (b - a);
}

// Vector
template<typename T>
std::vector<T> interpolateLinear(const std::vector<T>& a, const std::vector<T>& b, double alpha) {
    if (a.size() != b.size())
        throw std::runtime_error("Vector sizes do not match for interpolation.");
    std::vector<T> result(a.size());
    for (size_t i = 0; i < a.size(); ++i)
        result[i] = interpolateLinear(a[i], b[i], alpha);
    return result;
}

// Matrix
template<typename T>
std::vector<std::vector<T>> interpolateLinear(const std::vector<std::vector<T>>& a,
                                              const std::vector<std::vector<T>>& b,
                                              double alpha) {
    if (a.size() != b.size())
        throw std::runtime_error("Matrix row count does not match for interpolation.");
    std::vector<std::vector<T>> result(a.size());
    for (size_t i = 0; i < a.size(); ++i)
        result[i] = interpolateLinear(a[i], b[i], alpha);
    return result;
}

// Resolve interpolation overload
template<typename T>
std::function<T(const T&, const T&, double)> getDefaultInterpolator() {
    return [](const T& a, const T& b, double alpha) {
        return interpolateLinear(a, b, alpha);
    };
}

// ======= InterpolatedParameter Class =======

template<typename T>
class InterpolatedParameter {
public:
    std::map<double, T> timeValues;
    std::function<T(const T&, const T&, double)> interpolate;

    InterpolatedParameter(std::function<T(const T&, const T&, double)> interp_func = nullptr)
        : interpolate(interp_func ? interp_func : getDefaultInterpolator<T>()) {}

    void setTimeValue(double t, const T& value) {
        timeValues[t] = value;
    }

    bool isConstant() const {
        return timeValues.size() == 1;
    }

    T getValue(double time) const {
        if (timeValues.empty())
            throw std::runtime_error("No values defined for interpolation.");

        auto it = timeValues.upper_bound(time);
        if (it == timeValues.begin())
            return it->second;
        if (it == timeValues.end())
            return std::prev(it)->second;

        auto it2 = it;
        auto it1 = std::prev(it);
        double t1 = it1->first;
        double t2 = it2->first;
        double alpha = (time - t1) / (t2 - t1);

        return interpolate(it1->second, it2->second, alpha);
    }

    T operator()(double time) const {
        return getValue(time);
    }
};

