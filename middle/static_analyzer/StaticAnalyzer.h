#include "../../front/ast/node.h"
#include "ScopedVariables.h"
#include <vector>

#ifndef COMPILER_STATICANALYZER_H
#define COMPILER_STATICANALYZER_H

class StaticAnalyzer {
private:
    Program &program;
    ScopedVariables &scopedVariables;
public:
    StaticBuilder(Program &program) : program(program) {}


};

#endif //COMPILER_STATICANALYZER_H
