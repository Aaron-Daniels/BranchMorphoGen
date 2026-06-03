#ifndef RANDOMUTILS_H
#define RANDOMUTILS_H

#include <random>

namespace RandomUtils {

    // Thread-local generator ensures each thread has its own RNG
    inline thread_local std::mt19937 generator;

    // Call this once per thread to seed its own generator
    inline void InitializeGenerator(unsigned int seed) {
        generator.seed(seed);
    }

    // Access thread-local generator directly
    inline std::mt19937& GetGenerator() {
        return generator;
    }

    // Integer uniform distribution
    inline int IntUniform(int a, int b) {
        std::uniform_int_distribution<> dist(a, b);
        return dist(generator);
    }

    // Random sign: returns -1 or +1
    inline int RandomSign() {
        return IntUniform(0, 1) == 0 ? -1 : 1;
    }

    // Uniform in [0, 1)
    inline double UnitUniform() {
        static thread_local std::uniform_real_distribution<> dist(0.0, 1.0);
        return dist(generator);
    }

    // Uniform in [a, b)
    inline double Uniform(double a, double b) {
        std::uniform_real_distribution<> dist(a, b);
        return dist(generator);
    }

    // Normal distribution with mean 0 and stddev 1
    inline double UnitNormal() {
        std::normal_distribution<> dist(0.0, 1.0);
        return dist(generator);
    }

    // Normal distribution with custom mean and stddev
    inline double Normal(double mean, double stddev) {
        if(stddev<=1.0e-12) return mean;
        std::normal_distribution<> dist(mean, stddev);
        return dist(generator);
    }

    // Exponential distribution with rate lambda
    inline double Exponential(double lambda) {
        std::exponential_distribution<> dist(lambda);
        return dist(generator);
    }

    // Poisson distribution with given mean
    inline int Poisson(double mean) {
        std::poisson_distribution<> dist(mean);
        return dist(generator);
    }

    // Log-normal distribution
    inline double LogNormal(double m, double s) {
        std::lognormal_distribution<> dist(m, s);
        return dist(generator);
    }

} // namespace RandomUtils

#endif // RANDOMUTILS_H
