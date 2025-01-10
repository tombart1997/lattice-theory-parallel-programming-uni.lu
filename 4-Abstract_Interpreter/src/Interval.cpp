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
    Interval join(const Interval& other) const {
        return Interval(std::min(lower, other.lower), std::max(upper, other.upper));
    }



    // Arithmetic operations

    Interval add(const Interval& other) {
        int newLower = lower + other.lower;
        int newUpper = upper + other.upper;

        // Detect integer overflow
        if ((lower > 0 && other.lower > 0 && newLower < lower) ||  // Overflow check
            (upper > 0 && other.upper > 0 && newUpper < upper)) {  
            std::cerr << "[WARNING] Possible integer overflow detected in addition!\n";
            return Interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()); // Return top interval
        }

        return Interval(newLower, newUpper);
    }

    Interval subtract(const Interval& other) {
        // Compute lower and upper bounds
        int newLower = lower - other.upper;
        int newUpper = upper - other.lower;

        // Check for overflow or underflow
        if (newLower > lower || newUpper < upper) {
            std::cerr << "[WARNING] Possible integer overflow detected in subtraction!\n";
            return Interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        }

        return Interval(newLower, newUpper);
    }



    Interval multiply(const Interval& other) {
        int vals[] = { lower * other.lower, lower * other.upper, upper * other.lower, upper * other.upper };
        int minVal = *std::min_element(vals, vals + 4);
        int maxVal = *std::max_element(vals, vals + 4);

        if (minVal < std::numeric_limits<int>::min() || maxVal > std::numeric_limits<int>::max()) {
            std::cerr << "[WARNING] Possible integer overflow detected in multiplication!\n";
            return Interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        }

        return Interval(minVal, maxVal);
    }

    Interval divide(const Interval& other) {
        if (other.lower <= 0 && other.upper >= 0) {
            std::cerr << "Error: Division by zero detected in interval analysis!\n";
            return *this;
        }
        int vals[] = { lower / other.lower, lower / other.upper, upper / other.lower, upper / other.upper };
        return Interval(*std::min_element(vals, vals + 4), *std::max_element(vals, vals + 4));
    }
    
    Interval intersect(const Interval& other) {
        int newLower = std::max(lower, other.lower);
        int newUpper = std::min(upper, other.upper);

        // If the intersection is invalid, return an empty interval
        if (newLower > newUpper) {
            return Interval(std::numeric_limits<int>::max(), std::numeric_limits<int>::min()); // Empty set
        }

        return Interval(newLower, newUpper);    
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