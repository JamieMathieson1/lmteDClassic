#pragma once

#include <string>

class Compiler {
public:
    static int run(const std::string& inputPath, const std::string& outputPath);
};