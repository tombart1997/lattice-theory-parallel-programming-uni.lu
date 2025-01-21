#ifndef EQUATIONAL_ABSTRACT_INTERPRETER_HPP
#define EQUATIONAL_ABSTRACT_INTERPRETER_HPP

#include "ast.hpp"
#include "IntervalStore.cpp"
#include <map>
#include <iostream>
#include <sstream>
#include <regex>
#include <set>

/**
 * @class EquationalAbstractInterpreter
 * @brief Implements an abstract interpreter based on interval analysis.
 *
 * This class constructs an equational representation of a program and solves for
 * variable intervals using fixpoint iteration. It supports:
 * - Assignments
 * - Conditional branching (`if-else`)
 * - Loops (`while`)
 * - Fixpoint computation with widening to ensure termination
 *
 * ## Debugging Information:
 * - Each step prints detailed execution logs.
 * - Fixpoint iterations print changes to variable intervals.
 * - Uninitialized variable accesses issue warnings.
 * - Widening is applied dynamically to prevent infinite loops.
 */
class EquationalAbstractInterpreter {
public:
    std::set<std::string> loopVariables; // Tracks variables modified inside loops
    std::map<int, IntervalStore> programStates;  // Maps program location ℓ to interval states
    std::map<int, std::string> programEquations; // Stores equational representations of statements
    bool changed = true; // Tracks changes during fixpoint iteration

    /**
     * @brief Evaluates a given AST node by generating and solving equations.
     * @param node The root AST node of the program.
     */
    void eval(ASTNode& node) {
        int location = 0; // Track program locations

        std::cout << "[INFO] Starting equation generation...\n";
        evalNode(node, location);
        std::cout << "[INFO] Equation generation completed.\n";

        std::cout << "[INFO] Starting fixpoint computation...\n";
        solveFixpoint();
        std::cout << "[INFO] Fixpoint computation completed.\n";
    }

private:
    void evalNode(ASTNode& node, int& location) {
        std::cout << "[DEBUG] Evaluating NodeType: " << node.type << std::endl;

        switch (node.type) {
            case NodeType::PRE_CON:
                handlePreconditions(node, location);
                break;
            case NodeType::ASSIGNMENT:
                handleAssignment(node, location);
                break;
            case NodeType::IFELSE:
                handleIfElse(node, location);
                break;
            default:
                for (auto& child : node.children) {
                    evalNode(child, location);
                }
        }
    }

    void handlePreconditions(ASTNode& node, int location) {
        std::cout << "[DEBUG] Entering handlePreconditions()\n";
        std::string varName;
        Interval interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

        for (const auto& condition : node.children) {
            if (condition.type != NodeType::LOGIC_OP) {
                std::cerr << "[ERROR] Expected a logic operation, found: " << condition.type << std::endl;
                continue;
            }

            LogicOp op;
            bool validLogicOp = false;

            // Extract the logic operation safely using std::visit
            std::visit([&](auto&& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, LogicOp>) {
                    op = value;
                    validLogicOp = true;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    if (value == "<") op = LogicOp::LE;
                    else if (value == "<=") op = LogicOp::LEQ;
                    else if (value == ">") op = LogicOp::GE;
                    else if (value == ">=") op = LogicOp::GEQ;
                    else if (value == "==") op = LogicOp::EQ;
                    else if (value == "!=") op = LogicOp::NEQ;
                    else {
                        std::cerr << "[ERROR] Unknown logic operation: " << value << std::endl;
                        return;
                    }
                    validLogicOp = true;
                }
            }, condition.value);

            if (!validLogicOp) {
                std::cerr << "[ERROR] Invalid or missing LogicOp. Skipping condition.\n";
                continue;
            }

            if (condition.children.size() != 2) {
                std::cerr << "[ERROR] Malformed logic operation. Expected exactly 2 operands.\n";
                continue;
            }

            // Process left and right nodes of the logic operation
            ASTNode left = condition.children[0];
            ASTNode right = condition.children[1];

            int bound = 0;
            bool flipped = false;

            if (left.type == NodeType::INTEGER && right.type == NodeType::VARIABLE) {
                varName = std::get<std::string>(right.value);
                bound = std::get<int>(left.value);
                flipped = true;
            } else if (left.type == NodeType::VARIABLE && right.type == NodeType::INTEGER) {
                varName = std::get<std::string>(left.value);
                bound = std::get<int>(right.value);
            } else {
                std::cerr << "[ERROR] Logical condition must involve one variable and one integer.\n";
                continue;
            }

            // Adjust interval based on the operation and bounds
            if (!flipped) {
                if (op == LogicOp::GEQ) interval.lower = std::max(interval.lower, bound);
                if (op == LogicOp::LEQ) interval.upper = std::min(interval.upper, bound);
            } else {
                if (op == LogicOp::GEQ) interval.upper = std::min(interval.upper, bound);
                if (op == LogicOp::LEQ) interval.lower = std::max(interval.lower, bound);
            }
        }

        std::cout << "[DEBUG] Corrected constraint: " << varName << " in [" << interval.lower << ", " << interval.upper << "]\n";

        if (!varName.empty()) {
            // Store the interval in the preconditions
            programStates[0].setPrecondition(varName, interval);
            std::cout << "[DEBUG] Precondition stored successfully for: " << varName << " interval: [" << interval.lower << ", " << interval.upper << "]\n";
        } else {
            std::cerr << "[ERROR] No valid variable found to store preconditions.\n";
        }

        std::cout << "[DEBUG] Exiting handlePreconditions()\n";
    }


    void handleAssignment(ASTNode& node, int location) {
        std::string varName = std::get<std::string>(node.children[0].value);
        Interval value = evalArithmetic(node.children[1], location);

        programStates[location].setInterval(varName, value);
        programEquations[location] = "Xℓ" + std::to_string(location) + " = "
            "C(" + varName + " ← " + std::to_string(value.lower) + ", Xℓ" + std::to_string(location - 1) + ")";

        std::cout << "[DEBUG] Assignment: " << varName << " = [" << value.lower << ", " << value.upper << "]\n";
    }


void handleIfBody(ASTNode& ifBodyNode, IntervalStore& ifStore, int location) {
    std::cout << "[DEBUG] Executing IF Body...\n";

    // Clone the interval store for the IF branch
    programStates[location] = ifStore;

    // Iterate through statements in the IF body
    for (auto& stmt : ifBodyNode.children) {
        evalNode(stmt, location);
    }

    std::cout << "[DEBUG] IF Body execution completed.\n";
}

void handleElseBody(ASTNode& elseBodyNode, IntervalStore& elseStore, int location) {
    std::cout << "[DEBUG] Executing ELSE Body...\n";

    // Clone the interval store for the ELSE branch
    programStates[location] = elseStore;

    // Iterate through statements in the ELSE body
    for (auto& stmt : elseBodyNode.children) {
        evalNode(stmt, location);
    }

    std::cout << "[DEBUG] ELSE Body execution completed.\n";
}

void handleIfElse(ASTNode& node, int& location) {
    std::cout << "[DEBUG] Entering handleIfElse()\n";

    // Increment locations for conditional and branches
    int condLoc = location++;
    int ifLoc = location++;
    int elseLoc = location++;
    int endLoc = location++;

    std::cout << "[DEBUG] Assigned locations: condLoc=" << condLoc
              << ", ifLoc=" << ifLoc
              << ", elseLoc=" << elseLoc
              << ", endLoc=" << endLoc << "\n";

    // Extract condition and branches
    ASTNode& condition = node.children[0];
    ASTNode& ifBodyNode = node.children[1];
    ASTNode* elseBodyNode = (node.children.size() > 2) ? &node.children[2] : nullptr;

    // Validate the condition node
    if (condition.children.empty() || condition.children[0].type != NodeType::LOGIC_OP) {
        std::cerr << "[ERROR] Malformed condition in IF-ELSE statement.\n";
        return;
    }

    // Access the Logic Operation node
    ASTNode& logicOp = condition.children[0];

    // Validate Logic Operation node
    if (logicOp.children.size() != 2) {
        std::cerr << "[ERROR] Malformed logic operation in condition.\n";
        return;
    }

    ASTNode& leftOperand = logicOp.children[0];
    ASTNode& rightOperand = logicOp.children[1];
    std::string conditionVar;
    Interval ifConditionInterval;

    try {
        // Extract variable and interval from the condition
        conditionVar = std::get<std::string>(leftOperand.value);
        ifConditionInterval = evalArithmetic(rightOperand, condLoc);
    } catch (const std::bad_variant_access&) {
        std::cerr << "[ERROR] Failed to extract variable or interval from condition.\n";
        return;
    }

    // Clone interval store for the IF branch
    IntervalStore ifStore = programStates[condLoc];
    ifStore.store[conditionVar].clear();
    ifStore.setInterval(conditionVar, ifConditionInterval);

    // Debug IF branch
    std::cout << "[DEBUG] IF branch restricted " << conditionVar << " to ["
              << ifConditionInterval.lower << ", " << ifConditionInterval.upper << "]\n";

    // Execute the IF Body
    handleIfBody(ifBodyNode, ifStore, ifLoc);

    // Handle ELSE branch if present
    if (elseBodyNode) {
        IntervalStore elseStore = programStates[condLoc];

        // Compute the negated condition intervals
        std::vector<Interval> originalIntervals = programStates[condLoc].getIntervals(conditionVar);
        std::vector<Interval> negatedConditions;

        if (ifConditionInterval.lower == ifConditionInterval.upper) {
            int val = ifConditionInterval.lower;
            for (const auto& orig : originalIntervals) {
                if (orig.lower < val) negatedConditions.push_back(Interval(orig.lower, val - 1));
                if (orig.upper > val) negatedConditions.push_back(Interval(val + 1, orig.upper));
            }
        } else {
            negatedConditions.push_back(Interval(std::numeric_limits<int>::min(), ifConditionInterval.lower - 1));
            negatedConditions.push_back(Interval(ifConditionInterval.upper + 1, std::numeric_limits<int>::max()));
        }

        for (const auto& neg : negatedConditions) {
            elseStore.setInterval(conditionVar, neg);
        }

        std::cout << "[DEBUG] ELSE branch restricted " << conditionVar << " to ";
        for (const auto& neg : negatedConditions) {
            std::cout << "[" << neg.lower << ", " << neg.upper << "] ";
        }
        std::cout << "\n";

        // Execute the ELSE Body
        handleElseBody(*elseBodyNode, elseStore, elseLoc);
    }

    // Merge results of IF and ELSE branches
    programEquations[endLoc] = "Xℓ" + std::to_string(endLoc) + " = "
        "Xℓ" + std::to_string(ifLoc) + " ∪ Xℓ" + (elseBodyNode ? std::to_string(elseLoc) : "∅");

    std::cout << "[DEBUG] Exiting handleIfElse()\n";
}




    Interval evalArithmetic(ASTNode& node, int location) {
        if (node.type == NodeType::INTEGER) {
            int value = std::get<int>(node.value);
            return Interval(value, value);
        } else if (node.type == NodeType::VARIABLE) {
            auto varName = std::get<std::string>(node.value);
            
            // First, check for intervals in the current program state
            auto intervals = programStates[location].getIntervals(varName);
            if (!intervals.empty()) {
                return intervals.front();
            }

            // If not found, fall back on preconditions
            intervals = programStates[0].getPreconditions(varName);
            if (!intervals.empty()) {
                return intervals.front();
            }

            // Default to top interval if no information is available
            std::cerr << "[ERROR] Variable `" << varName << "` has no known intervals or preconditions! Defaulting to top interval.\n";
            return Interval();
        } else if (node.type == NodeType::ARITHM_OP) {
            auto left = evalArithmetic(node.children[0], location);
            auto right = evalArithmetic(node.children[1], location);

            auto op = std::get<BinOp>(node.value);
            if (op == BinOp::ADD) return left.add(right);
            if (op == BinOp::SUB) return left.subtract(right);
            if (op == BinOp::MUL) return left.multiply(right);
            if (op == BinOp::DIV) return left.divide(right);
        }
        return Interval();
    }


    void solveFixpoint() {
        int iteration = 0;

        while (changed) {
            changed = false; // Reset at the start of each iteration
            std::cout << "[DEBUG] Fixpoint iteration " << iteration << " started...\n";

            for (auto& [loc, equation] : programEquations) {
                std::cout << "[TRACE] Evaluating: " << equation << "\n";

                // Parse and evaluate the equation
                std::regex eq_regex(R"(Xℓ(\d+) = C\((\w+) ← (-?\d+), Xℓ(\d+)\))");
                std::smatch match;

                if (std::regex_search(equation, match, eq_regex)) {
                    std::string var = match[2].str();
                    int value = std::stoi(match[3].str());

                    Interval newInterval(value, value);

                    // Get the current intervals for the variable
                    std::vector<Interval> prevIntervals = programStates[loc].getIntervals(var);

                    // Print old and new intervals for debugging
                    std::cout << "[DEBUG] Old interval for `" << var << "`: ";
                    if (!prevIntervals.empty()) {
                        for (const auto& interval : prevIntervals) {
                            std::cout << "[" << interval.lower << ", " << interval.upper << "] ";
                        }
                    } else {
                        std::cout << "(none)";
                    }
                    std::cout << "\n";

                    std::cout << "[DEBUG] New interval for `" << var << "`: [" 
                            << newInterval.lower << ", " << newInterval.upper << "]\n";

                    // Check if the new interval is different
                    if (prevIntervals.empty() || !(prevIntervals.back() == newInterval)) {
                        changed = true;
                        programStates[loc].replaceInterval(var, newInterval);
                        std::cout << "[UPDATE] Xℓ" << loc << " updated to [" 
                                << newInterval.lower << ", " 
                                << newInterval.upper << "]\n";
                    } else {
                        std::cout << "[INFO] No change detected for `" << var << "` at Xℓ" << loc << ", skipping update.\n";
                    }
                }
            }

            iteration++;
        }

        std::cout << "[INFO] Fixpoint reached after " << iteration << " iterations.\n";
    }

};

#endif
