#ifndef ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP
#define ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP

#include "Interval.cpp"
#include <map>
#include <string>

class IntervalStore {
public:
    std::map<std::string, Interval> store;

    // Set an interval for a variable
    void setInterval(const std::string& var, const Interval& interval) {
        store[var] = interval;
    }

    // Join two interval stores (used for merging if-else branches)
    void join(const IntervalStore& other) {
        for (const auto& pair : other.store) {
            const std::string& var = pair.first;
            const Interval& otherInterval = pair.second;

            if (store.find(var) != store.end()) {
                store[var] = store[var].join(otherInterval);
            } else {
                store[var] = otherInterval;
            }
        }
    }


    // Get an interval for a variable
    Interval getInterval(const std::string& var) {
        if (store.find(var) != store.end()) {
            return store[var];
        }
        return Interval(); // Default to (-∞, +∞)
    }

    

    void print() {
        for (const auto& pair : store) {
            std::cout << pair.first << " -> ";
            pair.second.print();
            std::cout << "\n";
        }
    }
};

#endif
