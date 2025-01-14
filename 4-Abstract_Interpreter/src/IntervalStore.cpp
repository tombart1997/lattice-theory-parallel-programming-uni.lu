#ifndef ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP
#define ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP

#include "Interval.cpp"
#include <map>
#include <vector>
#include <string>
#include <algorithm>

class IntervalStore {
public:
    std::map<std::string, std::vector<Interval>> store;
    std::map<std::string, std::vector<Interval>> preconditions; // Store preconditions separately

    void setInterval(const std::string& var, Interval newInterval) {
    std::vector<Interval>& existingIntervals = store[var];

    // Try merging with existing intervals
    bool merged = false;
    for (auto& existing : existingIntervals) {
        if (existing.upper + 1 >= newInterval.lower && newInterval.upper + 1 >= existing.lower) {
            // Merge overlapping/adjacent intervals
            existing.lower = std::min(existing.lower, newInterval.lower);
            existing.upper = std::max(existing.upper, newInterval.upper);
            merged = true;
            break;
        }
    }

    if (!merged) {
        // If no merging happened, store separately
        existingIntervals.push_back(newInterval);
    }

    // Sort intervals for consistency
    std::sort(existingIntervals.begin(), existingIntervals.end(), [](const Interval& a, const Interval& b) {
        return a.lower < b.lower;
    });

    }

    // **New Function: Get Precondition Intervals**
    std::vector<Interval> getPreconditions(const std::string& var) {
        if (preconditions.find(var) == preconditions.end()) {
            return { Interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()) };  // Default to [-∞, ∞]
        }
        return preconditions[var];
    }

    // **New Function: Store Precondition**
    void setPrecondition(const std::string& var, Interval precond) {
        preconditions[var].clear();
        preconditions[var].push_back(precond);
    }



    // Get all intervals for a variable
    std::vector<Interval> getIntervals(const std::string& var) {
        if (store.find(var) == store.end()) return {};
        return store[var];
    }

    // Join two interval stores (now handles multiple intervals correctly)
    void join(const IntervalStore& other) {
        for (const auto& pair : other.store) {
            const std::string& var = pair.first;
            for (const auto& interval : pair.second) {
                setInterval(var, interval);
            }
        }
    }

    // Debugging utility
    void print() {
        for (const auto& pair : store) {
            std::cout << pair.first << " -> { ";
            for (const auto& interval : pair.second) {
                interval.print();
                std::cout << " ";
            }
            std::cout << "}\n";
        }
    }
};

#endif
