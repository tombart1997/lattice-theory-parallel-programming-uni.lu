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
            case NodeType::WHILELOOP:
                handleWhileLoop(node, location);
                break;
            case NodeType::POST_CON:
                checkAssertion(node, location);
                break;
            default:
                for (auto& child : node.children) {
                    evalNode(child, location);
                }
        }
    }

void checkAssertion(ASTNode& node, int location) {
    if (node.children.empty()) {
        std::cerr << "[ERROR] Assertion check failed! No condition found.\n";
        return;
    }
    ASTNode& condition = node.children[0];
    Interval left = evalArithmetic(condition.children[0], location);
    Interval right = evalArithmetic(condition.children[1], location);
    LogicOp op = std::get<LogicOp>(condition.value);
    bool result = false;

    switch (op) {
        case LogicOp::EQ:
            result = left.is_equal(right);
            break;
        case LogicOp::LEQ:
            result = left.upper <= right.upper;
            break;
        case LogicOp::GEQ:
            result = left.lower >= right.lower;
            break;
        case LogicOp::LE:
            result = left.upper < right.lower;
            break;
        case LogicOp::GE:
            result = left.lower > right.upper;
            break;
        default:
            std::cerr << "[ERROR] Unsupported logic operation in assertion.\n";
            return;
    }

    if (result) {
        std::cout << "[OK] Assertion passed. ";
        left.print();
        std::cout << " " << op << " ";
        right.print();
        std::cout << "\n";
    } else {
        std::cerr << "[FAIL] Assertion failed! Condition: ";
        left.print();
        std::cerr << " " << op << " ";
        right.print();
        std::cerr << "\n";
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

        programStates[location].replaceInterval(varName, value);
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

    // Define program locations
    int condLoc = location++;
    int ifLoc = location++;
    int elseLoc = location++;
    int endLoc = location++;
    // Extract components of the if-else structure
    ASTNode& condition = node.children[0];
    ASTNode& ifBodyNode = node.children[1];
    ASTNode* elseBodyNode = (node.children.size() > 2) ? &node.children[2] : nullptr;
    IntervalStore ifStore = programStates[condLoc];
    IntervalStore elseStore = programStates[condLoc];

    // Process the condition for the IF branch

    Interval conditionInterval = evalArithmetic(ifBodyNode, condLoc);
    std::cout << "[DEBUG] Evaluated condition interval: [" 
              << conditionInterval.lower << ", " << conditionInterval.upper << "]\n";
    ifStore.setInterval(std::get<std::string>(condition.value), conditionInterval);

    // Evaluate the IF body
    handleIfBody(ifBodyNode, ifStore, ifLoc);

    // Process the ELSE branch if it exists
    if (elseBodyNode) {
        IntervalStore negatedElseStore = programStates[condLoc];

        // Handle negation logic for the condition
        negatedElseStore.setInterval(
            std::get<std::string>(condition.value),
            Interval(std::numeric_limits<int>::min(), conditionInterval.lower - 1)
        );
        handleElseBody(*elseBodyNode, negatedElseStore, elseLoc);

    }
    // Merge IF and ELSE intervals into the end location
    programStates[endLoc] = programStates[ifLoc];
    programStates[endLoc].join(programStates[elseLoc]);
} 



    Interval evalArithmetic(ASTNode& node, int location) {
        if (node.type == NodeType::INTEGER) {
            int value = std::get<int>(node.value);
            return Interval(value, value);
        } else if (node.type == NodeType::VARIABLE) {
            auto varName = std::get<std::string>(node.value);

            // Traverse backward to find the most recent interval
            int currentLoc = location;
            while (currentLoc >= 0) {
                auto intervals = programStates[currentLoc].getIntervals(varName);
                if (!intervals.empty()) {
                    return intervals.front();
                }
                --currentLoc;
            }

            // Fallback to preconditions if no interval was found
            auto preconditions = programStates[0].getPreconditions(varName);
            if (!preconditions.empty()) {
                std::cout << "[DEBUG] Using preconditions for `" << varName << "`: ";
                preconditions.front().print();
                std::cout << "\n";
                return preconditions.front();
            }

            // Default to the top interval [-∞, ∞]
            std::cerr << "[ERROR] Variable `" << varName 
                    << "` has no known intervals or preconditions! Defaulting to top interval.\n";
            return Interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
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
        changed = false;
        std::cout << "[DEBUG] Fixpoint iteration " << iteration << " started...\n";

        for (auto& [loc, equation] : programEquations) {
            std::cout << "[TRACE] Evaluating: " << equation << "\n";

            // Parse equation and update intervals
            std::regex eq_regex(R"(Xℓ(\d+) = C\((\w+) ← (-?\d+), Xℓ(\d+)\))");
            std::smatch match;

            if (std::regex_search(equation, match, eq_regex)) {
                std::string var = match[2].str();
                int value = std::stoi(match[3].str());
                Interval newInterval(value, value);

                // Retrieve previous intervals for comparison
                std::vector<Interval> prevIntervals = programStates[loc].getIntervals(var);

                std::cout << "[DEBUG] Old intervals for `" << var << "`: ";
                for (const auto& interval : prevIntervals) {
                    std::cout << "[" << interval.lower << ", " << interval.upper << "] ";
                }
                std::cout << "\n";

                // Replace interval if it changes
                if (prevIntervals.empty() || !(prevIntervals.back() == newInterval)) {
                    programStates[loc].replaceInterval(var, newInterval);
                    changed = true;
                    std::cout << "[UPDATE] Updated Xℓ" << loc << " to [" << newInterval.lower << ", " << newInterval.upper << "]\n";
                } else {
                    std::cout << "[INFO] No change for `" << var << "` at Xℓ" << loc << ".\n";
                }
            }
        }
        iteration++;
    }

    std::cout << "[INFO] Fixpoint reached after " << iteration << " iterations.\n";
}


   void handleWhileLoop(ASTNode& node, int& location) {
        std::cout << "[DEBUG] Entering handleWhileLoop()\n";
        int condLoc = location++;
        int bodyLoc = location++;
        int endLoc = location++;

        IntervalStore loopState = evaluateLoopCondition(node.children[0], condLoc);
        if (loopState.store.empty()) {
            std::cerr << "[ERROR] Failed to evaluate loop condition.\n";
            return;
        }

        IntervalStore finalState = processLoopBody(loopState, node.children[1], bodyLoc, condLoc);

        programStates[endLoc] = finalState;
        std::cout << "[DEBUG] Exiting handleWhileLoop()\n";
    }

    IntervalStore evaluateLoopCondition(ASTNode& conditionNode, int condLoc) {
        std::cout << "[DEBUG] Evaluating loop condition...\n";
        if (conditionNode.children.empty() || conditionNode.children[0].type != NodeType::LOGIC_OP) {
            std::cerr << "[ERROR] Malformed loop condition.\n";
            return IntervalStore();
        }

        ASTNode& logicOp = conditionNode.children[0];
        if (logicOp.children.size() != 2) {
            std::cerr << "[ERROR] Malformed logic operation in loop condition.\n";
            return IntervalStore();
        }

        ASTNode& leftOperand = logicOp.children[0];
        ASTNode& rightOperand = logicOp.children[1];
        std::string varName;
        Interval conditionInterval;

        try {
            varName = std::get<std::string>(leftOperand.value);
            conditionInterval = evalArithmetic(rightOperand, condLoc);
        } catch (const std::bad_variant_access&) {
            std::cerr << "[ERROR] Failed to extract variable or interval from condition.\n";
            return IntervalStore();
        }

        IntervalStore loopState;
        loopState.setInterval(varName, conditionInterval);
        std::cout << "[DEBUG] Condition variable: " << varName << " Interval: [" 
                  << conditionInterval.lower << ", " << conditionInterval.upper << "]\n";

        return loopState;
    }

    IntervalStore processLoopBody(IntervalStore& loopState, ASTNode& loopBody, int bodyLoc, int condLoc) {
        bool loopChanged = true;
        int iteration = 0;

        while (loopChanged) {
            loopChanged = false;
            IntervalStore iterState = loopState;

            evalNode(loopBody, bodyLoc);

            for (const auto& [var, intervals] : programStates[bodyLoc].store) {
                for (const Interval& interval : intervals) {
                    if (!loopState.setInterval(var, interval)) {
                        loopChanged = true;
                    }
                }
            }

            if (iteration > 5) {
                loopChanged = loopState.applyWidening();
            }

            iteration++;
        }

        std::cout << "[DEBUG] Loop fixpoint reached after " << iteration << " iterations.\n";
        return loopState;
    }


};

#endif
