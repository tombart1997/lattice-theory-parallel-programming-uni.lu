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

    // Check if the interval is empty
    bool isEmpty() const {
        return lower > upper;
    }



    // Join operation (Least Upper Bound in lattice theory)
    Interval join(const Interval& other) const {
        if (isEmpty()) return other;
        if (other.isEmpty()) return *this;

        //  Handle non-overlapping intervals **without force-merging**
        if (upper + 1 < other.lower || other.upper + 1 < lower) {
            std::cerr << "[WARNING] Non-overlapping intervals detected: ["
                    << lower << ", " << upper << "] and ["
                    << other.lower << ", " << other.upper << "]\n";
            return *this;  // Return the original interval to avoid incorrect expansion
        }

        return Interval(std::min(lower, other.lower), std::max(upper, other.upper));
    }


    // Arithmetic operations
    Interval add(const Interval& other) {
        int64_t newLower = (int64_t)lower + (int64_t)other.lower;
        int64_t newUpper = (int64_t)upper + (int64_t)other.upper;

        // Correct overflow detection
        if (newLower < std::numeric_limits<int>::min() || newUpper > std::numeric_limits<int>::max()) {
            std::cerr << "[WARNING] Integer overflow detected in addition!\n";
            return Interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()); 
        }

        return Interval((int)newLower, (int)newUpper);
    }


    Interval subtract(const Interval& other) {
        int newLower = lower - other.upper;
        int newUpper = upper - other.lower;

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
            std::cerr << "[ERROR] Division by zero detected in interval analysis! Returning top interval.\n";
            return Interval();  
        }

        int vals[] = { lower / other.lower, lower / other.upper, upper / other.lower, upper / other.upper };
        return Interval(*std::min_element(vals, vals + 4), *std::max_element(vals, vals + 4));
    }
    
    Interval intersect(const Interval& other) {
        int newLower = std::max(lower, other.lower);
        int newUpper = std::min(upper, other.upper);

        if (newLower > newUpper) {
            std::cerr << "[ERROR] Invalid intersection detected! Returning explicit empty interval.\n";
            return Interval(1, 0); // Explicit empty interval
        }

        return Interval(newLower, newUpper);    
    }

    Interval widen(const Interval& other) const {
        int widenedLower = (other.lower < lower) ? std::numeric_limits<int>::min() : lower;
        int widenedUpper = (other.upper > upper) ? std::numeric_limits<int>::max() : upper;
        return Interval(widenedLower, widenedUpper);
    }


    bool contains(int value) const {
        return lower <= value && value <= upper;
    }

    // Comparison operations
    bool is_less_than(const Interval& other) {
        return upper < other.lower;
    }

    bool is_greater_than(const Interval& other) {
        return lower > other.upper;
    }

    bool is_equal(const Interval& other) {
        return lower == other.lower && upper == other.upper;
    }

    bool operator==(const Interval& other) const {
        return lower == other.lower && upper == other.upper;
    }

    bool operator!=(const Interval& other) const {
        return !(*this == other);
    }

    void print() const {
            std::cout << "[" << lower << ", " << upper << "]";
    }
};

#endif
