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
        Interval interval(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());  // Initialize properly

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
                        std::cerr << "[ERROR] Unknown string logic operation: " << value << std::endl;
                        return;
                    }
                    validLogicOp = true;
                }
            }, condition.value);

            if (!validLogicOp) {
                std::cerr << "[ERROR] condition.value is not a valid LogicOp. Skipping.\n";
                continue;
            }

            std::cout << "[DEBUG] Extracted LogicOp: " << op << std::endl;

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
                if (op == LogicOp::GEQ) interval.lower = std::max(interval.lower, bound);  // a >= 0
                if (op == LogicOp::LEQ) interval.upper = std::min(interval.upper, bound);  // a <= 2
            } else {  // The condition was flipped (e.g., `2 >= a`)
                if (op == LogicOp::GEQ) interval.upper = std::min(interval.upper, bound);  // 2 >= a  →  a ≤ 2
                if (op == LogicOp::LEQ) interval.lower = std::max(interval.lower, bound);  // 0 <= a  →  a ≥ 0
            }

            std::cout << "[DEBUG] Corrected constraint: " << varName << " in [" << interval.lower << ", " << interval.upper << "]\n";
        }

        if (!varName.empty()) {
            intervalStore.setInterval(varName, interval);
            std::cout << "[DEBUG] Interval stored successfully for: " << varName << "\n";
        } else {
            std::cerr << "[ERROR] No valid variable found in precondition.\n";
        }

        std::cout << "[DEBUG] Exiting handlePreconditions()\n";
    }




    void handleAssignment(ASTNode& node) {
        if (node.children.size() < 2) {
            std::cerr << "[ERROR] Invalid assignment operation! Not enough children in AST node.\n";
            return;
        }

        std::string varName = std::get<std::string>(node.children[0].value);
        Interval value = evalArithmetic(node.children[1]);

        if (value.lower < std::numeric_limits<int>::min() || value.upper > std::numeric_limits<int>::max()) {
            std::cerr << "[WARNING] Possible overflow when assigning to " << varName << "!\n";
        }

        intervalStore.setInterval(varName, value);
    }



    Interval evalArithmetic(ASTNode& node) {
        if (node.type == NodeType::INTEGER) {
            int value = std::get<int>(node.value);
            return Interval(value, value);
        } 
        else if (node.type == NodeType::VARIABLE) {
            std::string varName = std::get<std::string>(node.value);
            return intervalStore.getInterval(varName);
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

    
void handleIfElse(ASTNode& node) {
    std::cout << "[DEBUG] Entering handleIfElse()" << std::endl;

    // Extract Condition and Branches
    if (node.children.size() < 2) {
        std::cerr << "[ERROR] Malformed if-else statement (not enough children)!" << std::endl;
        return;
    }

    ASTNode& condition = node.children[0];
    ASTNode& ifBodyNode = node.children[1];
    ASTNode* elseBodyNode = (node.children.size() > 2) ? &node.children[2] : nullptr;

    if (condition.children.empty()) {
        std::cerr << "[ERROR] Malformed if condition! No children." << std::endl;
        return;
    }

    ASTNode& logicOp = condition.children[0];

    std::cout << "[DEBUG] Extracted condition node: " << logicOp.type << std::endl;

    if (logicOp.children.size() != 2) {
        std::cerr << "[ERROR] Malformed logical operation in if statement (wrong number of operands)!" << std::endl;
        return;
    }

    std::string conditionVar;
    Interval conditionInterval;
    
    try {
        if (logicOp.children[0].type == NodeType::VARIABLE) {
            conditionVar = std::get<std::string>(logicOp.children[0].value);
            conditionInterval = evalArithmetic(logicOp.children[1]);
        } else if (logicOp.children[1].type == NodeType::VARIABLE) {
            conditionVar = std::get<std::string>(logicOp.children[1].value);
            conditionInterval = evalArithmetic(logicOp.children[0]);
        } else {
            std::cerr << "[ERROR] If condition does not involve a variable and an integer!" << std::endl;
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception while extracting if condition: " << e.what() << std::endl;
        return;
    }

    std::cout << "[DEBUG] Condition variable: " << conditionVar
              << " restricted to: " << conditionInterval.lower << " to " << conditionInterval.upper << std::endl;

    // Clone interval store for branches
    IntervalStore ifStore = intervalStore;
    IntervalStore elseStore = intervalStore;

    ifStore.setInterval(conditionVar, conditionInterval);
    std::cout << "[DEBUG] If-branch restricted to: " << conditionInterval.lower << " to " << conditionInterval.upper << std::endl;

    std::cout << "[DEBUG] Evaluating If-Body block." << std::endl;
    for (auto& stmt : ifBodyNode.children) {
        try {
            std::string assignedVar = std::get<std::string>(stmt.children[0].value);
            Interval assignedValue = evalArithmetic(stmt.children[1]);
            ifStore.setInterval(assignedVar, assignedValue);
            eval(stmt);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception while evaluating If-Body: " << e.what() << std::endl;
            return;
        }
    }

    Interval ifBranchInterval = ifStore.getInterval(conditionVar);

    // Restrict condition in else-branch (b != conditionValue → valid remaining range)
    Interval originalInterval = intervalStore.getInterval(conditionVar);
    Interval negatedCondition = originalInterval.intersect(Interval(
        std::numeric_limits<int>::min(), conditionInterval.lower - 1
    ).join(
        Interval(conditionInterval.upper + 1, std::numeric_limits<int>::max())
    ));


    std::cout << "[DEBUG] Original interval of " << conditionVar << " before negation: " 
              << originalInterval.lower << " to " << originalInterval.upper << std::endl;

    try {
        if (conditionInterval.lower == conditionInterval.upper) {
            int val = conditionInterval.lower;

            if (val == originalInterval.lower) {
                negatedCondition = Interval(val + 1, originalInterval.upper);
            } else if (val == originalInterval.upper) {
                negatedCondition = Interval(originalInterval.lower, val - 1);
            } else {
                negatedCondition = Interval(originalInterval.lower, val - 1)
                                       .join(Interval(val + 1, originalInterval.upper));
            }
        } else {
            std::cerr << "[ERROR] Unsupported condition negation due to range condition!" << std::endl;
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception while computing negated condition: " << e.what() << std::endl;
        return;
    }

    elseStore.setInterval(conditionVar, negatedCondition);
    std::cout << "[DEBUG] Else-branch restricted to: " << negatedCondition.lower << " to " << negatedCondition.upper << std::endl;

    std::cout << "[DEBUG] Evaluating Else-Body block." << std::endl;
    Interval elseBranchInterval;
    if (elseBodyNode) {
        for (auto& stmt : elseBodyNode->children) {
            try {
                std::string assignedVar = std::get<std::string>(stmt.children[0].value);
                Interval assignedValue = evalArithmetic(stmt.children[1]);
                elseStore.setInterval(assignedVar, assignedValue);
                eval(stmt);
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception while evaluating Else-Body: " << e.what() << std::endl;
                return;
            }
        }
        elseBranchInterval = elseStore.getInterval(conditionVar);
    } else {
        elseBranchInterval = originalInterval;
    }

    Interval mergedInterval = Interval(
        std::min(ifBranchInterval.lower, elseBranchInterval.lower),
        std::max(ifBranchInterval.upper, elseBranchInterval.upper)
    );


    if (mergedInterval.lower > mergedInterval.upper) {
        std::cerr << "[ERROR] Invalid merge! Setting default valid range.\n";
        mergedInterval = Interval(ifBranchInterval.lower, elseBranchInterval.upper);
    }

    intervalStore.setInterval(conditionVar, mergedInterval);
    std::cout << "[DEBUG] If-Else branches merged successfully: " << mergedInterval.lower << " to " << mergedInterval.upper << std::endl;
}




};

#endif