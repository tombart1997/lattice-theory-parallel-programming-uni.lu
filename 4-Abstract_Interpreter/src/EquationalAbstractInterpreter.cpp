#ifndef EQUATIONAL_ABSTRACT_INTERPRETER_HPP
#define EQUATIONAL_ABSTRACT_INTERPRETER_HPP

#include "ast.hpp"
#include "IntervalStore.cpp"
#include <map>
#include <vector>

/*
class EquationalAbstractInterpreter {
public:
    std::map<int, IntervalStore> stores;  // Stores for each program location
    bool changed = true; // Track changes during fixpoint iteration

    void eval(ASTNode& root) {
        std::cout << "[DEBUG] Starting Fixpoint Computation..." << std::endl;

        // Initialize interval store for location 0 (start of program)
        stores[0] = IntervalStore();

        while (changed) {
            changed = false;
            traverse(root, 0);
        }

        std::cout << "[DEBUG] Fixpoint reached." << std::endl;
    }

private:
    void traverse(ASTNode& node, int location) {
        std::cout << "[DEBUG] Evaluating location " << location << std::endl;
        
        IntervalStore& currentStore = stores[location];
        IntervalStore previousStore = currentStore;

        switch (node.type) {
            case NodeType::ASSIGNMENT:
                handleAssignment(node, currentStore);
                break;
            case NodeType::POST_CON:
                checkAssertion(node, currentStore);
                break;
            case NodeType::IFELSE:
                handleIfElse(node, location);
                break;
            default:
                for (auto& child : node.children) {
                    traverse(child, location + 1);
                }
        }

        // If store changed, continue fixpoint iteration
        if (previousStore.store != currentStore.store) {
            changed = true;
        }
    }

    void handleAssignment(ASTNode& node, IntervalStore& store) {
        std::string varName = std::get<std::string>(node.children[0].value);
        Interval value = evalArithmetic(node.children[1], store);

        store.setInterval(varName, value);
    }

    Interval evalArithmetic(ASTNode& node, IntervalStore& store) {
        if (node.type == NodeType::INTEGER) {
            int value = std::get<int>(node.value);
            return Interval(value, value);
        } 
        else if (node.type == NodeType::VARIABLE) {
            std::string varName = std::get<std::string>(node.value);

            // Fetch all intervals for the variable
            std::vector<Interval> intervals = store.getIntervals(varName);

            // If no interval exists, return a default top interval
            if (intervals.empty()) return Interval();

            // Merge all possible intervals into a single range
            Interval merged = intervals[0];
            for (const auto& interval : intervals) {
                merged = merged.join(interval);
            }
            return merged;
        }
        return Interval();
    }


    void checkAssertion(ASTNode& node, IntervalStore& store) {
        Interval left = evalArithmetic(node.children[0], store);
        Interval right = evalArithmetic(node.children[1], store);

        if (!left.is_equal(right)) {
            std::cerr << "[FAIL] Assertion failed!" << std::endl;
        } else {
            std::cout << "[OK] Assertion passed." << std::endl;
        }
    }

void handleIfElse(ASTNode& node, int location) {
    std::cout << "[DEBUG] Entering handleIfElse()\n";

    ASTNode& condition = node.children[0];
    ASTNode& ifBody = node.children[1];
    ASTNode* elseBody = (node.children.size() > 2) ? &node.children[2] : nullptr;

    std::string conditionVar = std::get<std::string>(condition.children[0].value);
    Interval conditionInterval = evalArithmetic(condition.children[1], stores[location]);

    std::cout << "[DEBUG] Condition: " << conditionVar 
              << " in [" << conditionInterval.lower << ", " << conditionInterval.upper << "]\n";

    // Clone the interval store for both branches
    IntervalStore ifStore = stores[location];
    IntervalStore elseStore = stores[location];

    // Apply condition interval for the if branch
    ifStore.setInterval(conditionVar, conditionInterval);
    traverse(ifBody, location + 1);

    // Compute negated condition for the else branch
    Interval originalInterval = stores[location].getMergedInterval(conditionVar);
    Interval negatedCondition;

    if (conditionInterval.lower == conditionInterval.upper) {
        // Exact match: "if (a == X)" means "else (a != X)"
        int val = conditionInterval.lower;
        negatedCondition = Interval(std::numeric_limits<int>::min(), val - 1)
                           .join(Interval(val + 1, std::numeric_limits<int>::max()));
    } else {
        // Range negation: "if (a in [L, U])" means "else (a < L or a > U)"
        negatedCondition = Interval(std::numeric_limits<int>::min(), conditionInterval.lower - 1)
                           .join(Interval(conditionInterval.upper + 1, std::numeric_limits<int>::max()));
    }

    elseStore.setInterval(conditionVar, negatedCondition);

    // Evaluate the else branch
    if (elseBody) {
        traverse(*elseBody, location + 2);
    }

    // Merge interval stores from both branches
    stores[location] = ifStore;
    if (elseBody) {
        stores[location].join(elseStore);
    }

    std::cout << "[DEBUG] If-Else merged: " 
              << stores[location].getMergedInterval(conditionVar).lower 
              << " to " 
              << stores[location].getMergedInterval(conditionVar).upper 
              << "\n";
}

};



*/

#endif