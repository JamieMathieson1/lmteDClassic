#pragma once

#include "Tree.h"

class Optimizer {
public:

    // Run all optimisation passes on the AST.
    void optimize(NodePtr& root);

private:

    // Constant folding.
    void foldExpressions(NodePtr& node);

    // Loop unrolling.
    void unrollLoops(NodePtr& node);

    // Dead code elimination.
    void eliminateDeadCode(NodePtr& node);
};
