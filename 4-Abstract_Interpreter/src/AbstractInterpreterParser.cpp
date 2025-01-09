#ifndef ABSTRACT_INTERPRETER_PARSER_HPP
#define ABSTRACT_INTERPRETER_PARSER_HPP

#include "ast.hpp"
#include <sstream>
#include <vector>
#include <iostream>

class AbstractInterpreterParser {
public:
    ASTNode parse(const std::string& input) {
        std::istringstream stream(input);
        std::string token;
        ASTNode root(NodeType::SEQUENCE); // Root AST node

        while (stream >> token) {
            if (token == "int") {
                std::string var;
                stream >> var; // Read variable name
                root.children.emplace_back(NodeType::DECLARATION, var);
            } else if (token == "=") {
                std::string var;
                int value;
                stream >> var >> value; // Read assignment
                ASTNode assignment(NodeType::ASSIGNMENT);
                assignment.children.emplace_back(NodeType::VARIABLE, var);
                assignment.children.emplace_back(NodeType::INTEGER, value);
                root.children.push_back(assignment);
            } else if (token == "assert") {
                std::string var;
                std::string op;
                int value;
                stream >> var >> op >> value; // Read assertion
                ASTNode assertNode(NodeType::POST_CON);
                assertNode.children.emplace_back(NodeType::VARIABLE, var);
                assertNode.children.emplace_back(NodeType::INTEGER, value);
                root.children.push_back(assertNode);
            }
        }
        return root;
    }
};

#endif
