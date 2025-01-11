#ifndef ABSTRACT_INTERPRETER_HPP
#define ABSTRACT_INTERPRETER_HPP

#include "ast.hpp"
#include "IntervalStore.cpp"

class AbstractInterpreter {
public:
    IntervalStore intervalStore;
    bool preconditionsProcessed = false;


    void eval(ASTNode& node) {
        if (!preconditionsProcessed && node.type == NodeType::SEQUENCE) {
            std::cout << "[DEBUG] Handling preconditions once.\n";
            for (auto& child : node.children) {
                if (child.type == NodeType::PRE_CON) {
                    handlePreconditions(child);
                }
            }
            preconditionsProcessed = true;
        }

        std::cout << "[DEBUG] Evaluating NodeType: " << node.type << std::endl;
        
        switch (node.type) {
            case NodeType::ASSIGNMENT:
                handleAssignment(node);
                break;
            case NodeType::POST_CON:
                checkAssertion(node);
                break;
            case NodeType::IFELSE:
                handleIfElse(node);
                break;
            default:
                for (auto& child : node.children) {
                    eval(child);
                }
        }
    }

private:


void handlePreconditions(ASTNode& node) {
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
        std::visit([&](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, LogicOp>) {
                op = value;
                validLogicOp = true;
            } 
            else if constexpr (std::is_same_v<T, std::string>) {
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
            std::cerr << "[ERROR] condition.value is not a valid LogicOp. Skipping.\n";
            continue;
        }

        if (condition.children.size() != 2) {
            std::cerr << "[ERROR] Malformed logic operation, expected 2 operands.\n";
            continue;
        }

        ASTNode left = condition.children[0];
        ASTNode right = condition.children[1];

        int bound = 0;
        bool flipped = false;

        if (left.type == NodeType::INTEGER && right.type == NodeType::VARIABLE) {
            varName = std::get<std::string>(right.value);
            bound = std::get<int>(left.value);
            flipped = true;
        } 
        else if (left.type == NodeType::VARIABLE && right.type == NodeType::INTEGER) {
            varName = std::get<std::string>(left.value);
            bound = std::get<int>(right.value);
        } 
        else {
            std::cerr << "[ERROR] Logical condition must involve one variable and one integer.\n";
            continue;
        }

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
        intervalStore.setPrecondition(varName, interval);  // Store precondition properly
        std::cout << "[DEBUG] Precondition stored successfully for: " << varName << " interval: [" << interval.lower << ", " << interval.upper << "]\n";
    } else {
        std::cerr << "[ERROR] No valid variable found in precondition.\n";
    }

    std::cout << "[DEBUG] Exiting handlePreconditions()\n";
}


void handleAssignment(ASTNode& node) {
    std::string varName = std::get<std::string>(node.children[0].value);
    Interval value = evalArithmetic(node.children[1]);

    // Debug output before assignment
    std::cout << "[DEBUG] Assigning variable: " << varName 
              << " new interval: [" << value.lower << ", " << value.upper << "]\n";

    // Instead of direct assignment, handle multiple intervals
    intervalStore.setInterval(varName, value);

    // Debug output after assignment
    std::vector<Interval> updatedIntervals = intervalStore.getIntervals(varName);
    std::cout << "[DEBUG] Updated intervals for " << varName << ": ";
    for (const auto& interval : updatedIntervals) {
        std::cout << "[" << interval.lower << ", " << interval.upper << "] ";
    }
    std::cout << "\n";
}



Interval evalArithmetic(ASTNode& node) {
    if (node.type == NodeType::INTEGER) {
        int value = std::get<int>(node.value);
        return Interval(value, value);
    } 
    else if (node.type == NodeType::VARIABLE) {
        std::string varName = std::get<std::string>(node.value);
        
        // Retrieve all stored intervals for the variable
        std::vector<Interval> intervals = intervalStore.getIntervals(varName);

        // If multiple intervals exist, handle them properly (choose logic)
        if (intervals.empty()) return Interval(); // Return top interval

        return intervals[0]; // Choose the first interval as default (adjust as needed)
    } 
    else if (node.type == NodeType::ARITHM_OP) {
        if (node.children.size() < 2) {
            std::cerr << "[ERROR] Malformed arithmetic operation! Not enough operands.\n";
            return Interval();
        }
        
        Interval left = evalArithmetic(node.children[0]);
        Interval right = evalArithmetic(node.children[1]);
        BinOp op = std::get<BinOp>(node.value);

        if (op == BinOp::ADD) return left.add(right);
        if (op == BinOp::SUB) return left.subtract(right);
        if (op == BinOp::MUL) return left.multiply(right);
        if (op == BinOp::DIV) {
            if (right.lower <= 0 && right.upper >= 0) {
                std::cerr << "[ERROR] Division by zero detected!" << std::endl;
                return Interval(); // Return top interval or propagate error.
            }
            return left.divide(right);
        }
    }
    return Interval();
}


    void checkAssertion(ASTNode& node) {
        if (node.children.empty()) {
            std::cerr << "[ERROR] Assertion check failed! No condition found.\n";
            return;
        }

        ASTNode& condition = node.children[0];

        if (condition.type != NodeType::LOGIC_OP) {
            std::cerr << "[ERROR] Assertion does not contain a valid logic operation.\n";
            return;
        }

        if (condition.children.size() < 2) {
            std::cerr << "[ERROR] Malformed assertion! Logic operation has too few children.\n";
            return;
        }

        Interval left = evalArithmetic(condition.children[0]);
        Interval right = evalArithmetic(condition.children[1]);
        LogicOp op = std::get<LogicOp>(condition.value);

        bool result = false;
        if (op == LogicOp::EQ) result = left.is_equal(right);
        if (op == LogicOp::LEQ) result = left.upper <= right.lower;
        if (op == LogicOp::GEQ) result = left.lower >= right.upper;
        if (op == LogicOp::LE) result = left.upper < right.lower;  
        if (op == LogicOp::GE) result = left.lower > right.upper;  


        if (result) {
            std::cout << "[OK] Assertion passed.\n";
        } else {
            std::cerr << "[FAIL] Assertion failed! Expected: ";
            left.print();
            std::cout << " " << op << " ";
            right.print();
            std::cout << "\n";
        }
    }


void handleIfBody(ASTNode& ifBodyNode, IntervalStore& ifStore) {
    std::cout << "[DEBUG] Executing IF Body\n";
    
    // Iterate through statements in the IF body
    for (auto& stmt : ifBodyNode.children) {
        eval(stmt);
    }

    std::cout << "[DEBUG] IF Body execution completed.\n";
}

void handleElseBody(ASTNode& elseBodyNode, IntervalStore& elseStore) {
    std::cout << "[DEBUG] Executing ELSE Body\n";
    
    // Iterate through statements in the ELSE body
    for (auto& stmt : elseBodyNode.children) {
        eval(stmt);
    }

    std::cout << "[DEBUG] ELSE Body execution completed.\n";
}



void handleIfElse(ASTNode& node) {
    std::cout << "[DEBUG] Entering handleIfElse()\n";

    // Extract condition and branches
    ASTNode& condition = node.children[0];
    ASTNode& ifBodyNode = node.children[1];
    ASTNode* elseBodyNode = (node.children.size() > 2) ? &node.children[2] : nullptr;

    // Extract the logic operation
    ASTNode& logicOp = condition.children[0];

    // **Step 1: Extract the variable and condition interval**
    std::string conditionVar;
    Interval ifConditionInterval;

    try {
        conditionVar = std::get<std::string>(logicOp.children[0].value);
        ifConditionInterval = evalArithmetic(logicOp.children[1]);
    } catch (const std::bad_variant_access&) {
        std::cerr << "[ERROR] Failed to extract variable or interval from condition.\n";
        return;
    }

    // **Debug Output**
    std::cout << "[DEBUG] IF Condition Variable: " << conditionVar << "\n";
    std::cout << "[DEBUG] Expected IF Interval: [" << ifConditionInterval.lower << ", " << ifConditionInterval.upper << "]\n";

    // Retrieve precondition intervals
    std::vector<Interval> preconditionIntervals = intervalStore.getPreconditions(conditionVar);

    // **Debug output**
    std::cout << "[DEBUG] Precondition Intervals for " << conditionVar << " : ";
    for (const auto& pre : preconditionIntervals) {
        std::cout << "[" << pre.lower << ", " << pre.upper << "] ";
    }
    std::cout << "\n";

    // **Step 2: Check if IF condition is valid based on preconditions**
    bool isIfConditionValid = false;
    for (const Interval& pre : preconditionIntervals) {
        if (pre.contains(ifConditionInterval.lower)) {
            isIfConditionValid = true;
            break;
        }
    }

    if (!isIfConditionValid) {
        std::cout << "[DEBUG] IF Condition for " << conditionVar << " is impossible due to precondition. Skipping if-branch.\n";
        if (elseBodyNode) handleElseBody(*elseBodyNode, intervalStore);
        return;
    }

    std::cout << "[DEBUG] IF Condition for " << conditionVar << " satisfies precondition. Continuing the IF Branch\n";

    // **Step 3: Clone the interval store for the IF-branch**
    IntervalStore ifStore = intervalStore;
    ifStore.store[conditionVar].clear();
    ifStore.setInterval(conditionVar, ifConditionInterval);

    std::cout << "[DEBUG] IF-branch restricted " << conditionVar << " to [" 
              << ifConditionInterval.lower << ", " << ifConditionInterval.upper << "]\n";

    // **Step 4: Execute IF Body**
    handleIfBody(ifBodyNode, ifStore);

    // **Step 5: Compute ELSE condition and execute ELSE Body**
    if (elseBodyNode) {
        IntervalStore elseStore = intervalStore;

        // Compute the negated condition
        std::vector<Interval> originalIntervals = intervalStore.getIntervals(conditionVar);
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

        std::cout << "[DEBUG] ELSE-branch restricted " << conditionVar << " to ";
        for (const auto& neg : negatedConditions) {
            std::cout << "[" << neg.lower << ", " << neg.upper << "] ";
        }
        std::cout << "\n";

        // Execute ELSE Body
        handleElseBody(*elseBodyNode, elseStore);
    }
}



};

#endif