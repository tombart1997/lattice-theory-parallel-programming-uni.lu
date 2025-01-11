#include <fstream>
#include <sstream>
#include "parser.hpp"
#include "ast.hpp"
#include "AbstractInterpreter.cpp"


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " tests/00.c" << std::endl;
        return 1;
    }

    std::ifstream f(argv[1]);
    if (!f.is_open()) {
        std::cerr << "[ERROR] cannot open the test file `" << argv[1] << "`." << std::endl;
        return 1;
    }

    std::ostringstream buffer;
    buffer << f.rdbuf();
    std::string input = buffer.str();
    f.close();

    std::cout << "Parsing program `" << argv[1] << "`..." << std::endl;
    // Use the parser to generate the AST
    AbstractInterpreterParser parser;
    ASTNode ast = parser.parse(input);
    ast.print();

    // Pass AST to Abstract Interpreter for evaluation
    //EquationalAbstractInterpreter interpreter;
    AbstractInterpreter interpreter;
    interpreter.eval(ast);



    return 0;
}
