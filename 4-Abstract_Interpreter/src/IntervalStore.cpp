#ifndef ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP
#define ABSTRACT_INTERPRETER_INTERVAL_STORE_HPP

#include "Interval.cpp"
#include <map>
#include <string>

class IntervalStore {
public:
    std::map<std::string, Interval> store;

    // Set an interval for a variable (now properly intersects)
    void setInterval(const std::string& var, const Interval& interval) {
        if (store.find(var) != store.end()) {
            store[var] = store[var].intersect(interval);  // Restrict instead of replacing
        } else {
            store[var] = interval;
        }
    }

    // Join two interval stores (fixed expansion issue)
    void join(const IntervalStore& other) {
        for (const auto& pair : store) {
            const std::string& var = pair.first;
            if (other.store.find(var) != other.store.end()) {
                store[var] = store[var].join(other.store.at(var));
            }
            // ELSE: Keep original value, do NOT expand
        }
        
        // Add variables from `other` that don't exist in `this`
        for (const auto& pair : other.store) {
            if (store.find(pair.first) == store.end()) {
                store[pair.first] = pair.second;
            }
        }
    }

    // Get an interval for a variable (returns reference)
    Interval& getInterval(const std::string& var) {
        return store[var];  // Returns a reference instead of copying
    }

    // Debugging utility
    void print() {
        for (const auto& pair : store) {
            std::cout << pair.first << " -> ";
            pair.second.print();
            std::cout << "\n";
        }
    }
};

#endif
