// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <map>
#include <string>
#include <vector>

struct MetalUniform {
    std::string name, type;
    unsigned offset = 0, count = 1, stride = 0, matrixStride = 0;
    int sampler = -1;
};
struct MetalShaderStage {
    std::string source;
    std::vector<unsigned> attributes, samplers;
};
struct MetalShaderProgram {
    MetalShaderStage vertex, fragment;
    std::vector<MetalUniform> uniforms;
    unsigned uniformSize = 0;
};
bool Metal_TranslateShaders(const std::string &vertex, const std::string &fragment,
                            const std::map<std::string, unsigned> &attributes, MetalShaderProgram &result,
                            std::string &error);
