#ifndef ABSTRACT_INTERPRETER_INTERVAL_HPP
#define ABSTRACT_INTERPRETER_INTERVAL_HPP

#include <iostream>
#include <limits>
#include <algorithm>

class Interval {
public:
    int lower;
    int upper;

    // Default constructor represents the "top" interval (-∞, +∞)
    Interval(int l = std::numeric_limits<int>::min(), int u = std::numeric_limits<int>::max())
        : lower(l), upper(u) {}

    // Join operation (Least Upper Bound in lattice theory)
    Interval join(const Interval& other) {
        return Interval(std::min(lower, other.lower), std::max(upper, other.upper));
    }

    // Arithmetic operations
    Interval add(const Interval& other) {
        return Interval(lower + other.lower, upper + other.upper);
    }

    Interval subtract(const Interval& other) {
        return Interval(lower - other.upper, upper - other.lower);
    }

    Interval multiply(const Interval& other) {
        int vals[] = { lower * other.lower, lower * other.upper, upper * other.lower, upper * other.upper };
        return Interval(*std::min_element(vals, vals + 4), *std::max_element(vals, vals + 4));
    }

    Interval divide(const Interval& other) {
        if (other.lower <= 0 && other.upper >= 0) {
            std::cerr << "Error: Division by zero detected in interval analysis!\n";
            return *this;
        }
        int vals[] = { lower / other.lower, lower / other.upper, upper / other.lower, upper / other.upper };
        return Interval(*std::min_element(vals, vals + 4), *std::max_element(vals, vals + 4));
    }

    // Comparison operations (C♯ I J.K)
    bool is_less_than(const Interval& other) {
        return upper < other.lower;
    }

    bool is_greater_than(const Interval& other) {
        return lower > other.upper;
    }

    bool is_equal(const Interval& other) {
        return lower == other.lower && upper == other.upper;
    }

    void print() const {
        std::cout << "[" << lower << ", " << upper << "]";
    }
};

#endif
