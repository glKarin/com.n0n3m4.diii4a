// SPDX-License-Identifier: GPL-3.0-or-later
// Compile the engine's material language to native MSL. No GL driver is involved.
#include "metal_shader.h"
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_msl.hpp>
#include <regex>
#include <sstream>
#include <algorithm>

namespace {
struct Declaration {
    std::string type, name, array;
};
std::string preprocess(shaderc::Compiler &compiler, const std::string &input, shaderc_shader_kind kind) {
    std::string s = std::regex_replace(input, std::regex("#version[^\\n]*"), "#version 450");
    s = std::regex_replace(s, std::regex("#extension[^\\n]*"), "");
    shaderc::CompileOptions options;
    auto r = compiler.PreprocessGlsl(s, kind, "material", options);
    if (r.GetCompilationStatus() != shaderc_compilation_status_success)
        throw std::runtime_error(r.GetErrorMessage());
    return std::string(r.cbegin(), r.cend());
}
std::vector<Declaration> declarations(const std::string &source, const std::string &qualifier) {
    std::vector<Declaration> result;
    std::regex pattern("\\b" + qualifier +
                       "\\s+(?:(?:lowp|mediump|highp)\\s+)?(\\w+)\\s+(\\w+)\\s*(\\[[^;]+\\])?\\s*;");
    for (auto it = std::sregex_iterator(source.begin(), source.end(), pattern); it != std::sregex_iterator();
         ++it)
        result.push_back({(*it)[1], (*it)[2], (*it)[3]});
    return result;
}
std::string rewrite(const std::string &source, bool vertex, const std::vector<Declaration> &uniforms,
                    const std::map<std::string, unsigned> &attributes,
                    const std::map<std::string, unsigned> &varyings) {
    std::string s = std::regex_replace(
        source,
        std::regex("\\buniform\\s+(?:(?:lowp|mediump|highp)\\s+)?\\w+\\s+\\w+\\s*(?:\\[[^;]+\\])?\\s*;"), "");
    std::string header = "\nlayout(std140,set=0,binding=0) uniform ETQWUniforms {\n";
    for (const auto &u : uniforms)
        if (u.type.find("sampler") == std::string::npos)
            header += u.type + " " + u.name + u.array + ";\n";
    header += "};\n";
    unsigned sampler = 0;
    for (const auto &u : uniforms)
        if (u.type.find("sampler") != std::string::npos)
            header += "layout(set=1,binding=" + std::to_string(sampler++) + ") uniform " + u.type + " " +
                      u.name + u.array + ";\n";
    // ATI2/BC5 stores XY only. Reconstruct Z at sampling time, keeping the
    // original compressed mip chain on the GPU. Both RGB and AGB normal
    // conventions used by the game's material shaders receive the same XYZ.
    header += "vec4 etqwDecodeBC5(vec4 v, uint slot) { if ((etqw_bc5Mask & (1u << slot)) != 0u) { vec2 "
              "n=v.rg*2.0-1.0; v.b=sqrt(max(0.0,1.0-dot(n,n)))*0.5+0.5; v.a=v.r; } return v; }\n";
    sampler = 0;
    for (const auto &u : uniforms)
        if (u.type.find("sampler") != std::string::npos) {
            unsigned slot = sampler++;
            if (u.type != "sampler2D")
                continue;
            std::regex call("\\b(texture(?:Lod|Grad|Proj|ProjLod|ProjGrad|Offset|LodOffset|GradOffset)?|"
                            "texelFetch)\\s*\\(\\s*" +
                            u.name + "\\s*,");
            size_t start = 0;
            std::smatch match;
            while (start < s.size() && std::regex_search(s.cbegin() + start, s.cend(), match, call)) {
                size_t pos = start + match.position(), end = pos + s.substr(pos).find('(') + 1;
                int nesting = 1;
                while (end < s.size() && nesting) {
                    if (s[end] == '(')
                        ++nesting;
                    else if (s[end] == ')')
                        --nesting;
                    ++end;
                }
                if (nesting)
                    throw std::runtime_error("Unclosed texture sample");
                std::string wrapped =
                    "etqwDecodeBC5(" + s.substr(pos, end - pos) + ", " + std::to_string(slot) + "u)";
                s.replace(pos, end - pos, wrapped);
                start = pos + wrapped.size();
            }
        }
    // Insert after version; the preprocessor has already expanded all macros.
    auto versionEnd = s.find('\n');
    s.insert(versionEnd + 1, header);
    for (const auto &io : declarations(s, vertex ? "in" : "out")) {
        unsigned location = 0;
        if (vertex) {
            auto it = attributes.find(io.name);
            if (it != attributes.end())
                location = it->second;
        }
        s = std::regex_replace(s,
                               std::regex("\\b" + std::string(vertex ? "in" : "out") +
                                          "\\s+(?:(?:lowp|mediump|highp)\\s+)?" + io.type + "\\s+" + io.name +
                                          "\\b"),
                               "layout(location=" + std::to_string(location) + ") " +
                                   (vertex ? "in" : "out") + " " + io.type + " " + io.name);
    }
    for (const auto &io : declarations(s, vertex ? "out" : "in")) {
        auto it = varyings.find(io.name);
        if (it == varyings.end())
            continue;
        s = std::regex_replace(s,
                               std::regex("\\b" + std::string(vertex ? "out" : "in") +
                                          "\\s+(?:(?:lowp|mediump|highp)\\s+)?" + io.type + "\\s+" + io.name +
                                          "\\b"),
                               "layout(location=" + std::to_string(it->second) + ") " +
                                   (vertex ? "out" : "in") + " " + io.type + " " + io.name);
    }
    s = std::regex_replace(s, std::regex("\\bgl_VertexID\\b"), "gl_VertexIndex");
    s = std::regex_replace(s, std::regex("\\bgl_InstanceID\\b"), "gl_InstanceIndex");
    return s;
}
} // namespace

bool Metal_TranslateShaders(const std::string &vertex, const std::string &fragment,
                            const std::map<std::string, unsigned> &attributes, MetalShaderProgram &result,
                            std::string &error) {
    try {
        shaderc::Compiler compiler;
        std::string sources[] = {preprocess(compiler, vertex, shaderc_vertex_shader),
                                 preprocess(compiler, fragment, shaderc_fragment_shader)};
        std::vector<Declaration> uniforms;
        for (auto &s : sources)
            for (auto &u : declarations(s, "uniform")) {
                auto old = std::find_if(uniforms.begin(), uniforms.end(),
                                        [&](const Declaration &d) { return d.name == u.name; });
                if (old == uniforms.end())
                    uniforms.push_back(u);
                else if (old->type != u.type || old->array != u.array)
                    throw std::runtime_error("Uniform type mismatch: " + u.name);
            }
        uniforms.push_back({"uint", "etqw_bc5Mask", ""});
        if (std::none_of(uniforms.begin(), uniforms.end(),
                         [](const Declaration &u) { return u.type.find("sampler") == std::string::npos; }))
            uniforms.push_back({"vec4", "etqw_unused", ""});
        std::map<std::string, unsigned> varyings;
        unsigned nextVarying = 0;
        auto addVarying = [&](const Declaration &v) {
            if (varyings.count(v.name))
                return;
            varyings.emplace(v.name, nextVarying);
            unsigned slots =
                v.type.size() > 3 && v.type.substr(0, 3) == "mat" ? unsigned(v.type[3] - '0') : 1;
            if (!v.array.empty())
                slots *= std::max(1, atoi(v.array.c_str() + 1));
            nextVarying += slots;
        };
        for (auto &v : declarations(sources[0], "out"))
            addVarying(v);
        for (auto &v : declarations(sources[1], "in"))
            addVarying(v);
        for (int stage = 0; stage < 2; ++stage) {
            std::string glsl = rewrite(sources[stage], stage == 0, uniforms, attributes, varyings);
            shaderc::CompileOptions options;
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
            options.SetOptimizationLevel(shaderc_optimization_level_zero);
            auto spv = compiler.CompileGlslToSpv(
                glsl, stage == 0 ? shaderc_vertex_shader : shaderc_fragment_shader, "material", options);
            if (spv.GetCompilationStatus() != shaderc_compilation_status_success)
                throw std::runtime_error(spv.GetErrorMessage() + "\n" + glsl);
            spirv_cross::CompilerMSL msl(std::vector<uint32_t>(spv.cbegin(), spv.cend()));
            auto resources = msl.get_shader_resources();
            auto activeResources = msl.get_shader_resources(msl.get_active_interface_variables());
            if (stage == 0) {
                for (auto &ub : resources.uniform_buffers) {
                    auto &type = msl.get_type(ub.base_type_id);
                    result.uniformSize = msl.get_declared_struct_size(type);
                    for (unsigned i = 0; i < type.member_types.size(); ++i) {
                        MetalUniform u;
                        u.name = msl.get_member_name(type.self, i);
                        u.offset = msl.type_struct_member_offset(type, i);
                        auto &member = msl.get_type(type.member_types[i]);
                        auto decl = std::find_if(uniforms.begin(), uniforms.end(),
                                                 [&](const Declaration &d) { return d.name == u.name; });
                        if (decl != uniforms.end())
                            u.type = decl->type;
                        if (!member.array.empty()) {
                            u.count = member.array[0];
                            u.stride = msl.type_struct_member_array_stride(type, i);
                        }
                        if (member.columns > 1)
                            u.matrixStride = msl.type_struct_member_matrix_stride(type, i);
                        result.uniforms.push_back(u);
                    }
                }
                unsigned sampler = 0;
                for (auto &decl : uniforms)
                    if (decl.type.find("sampler") != std::string::npos) {
                        MetalUniform u;
                        u.name = decl.name;
                        u.type = decl.type;
                        u.sampler = sampler++;
                        result.uniforms.push_back(u);
                    }
            }
            auto &out = stage == 0 ? result.vertex : result.fragment;
            if (stage == 0)
                for (auto &a : activeResources.stage_inputs)
                    out.attributes.push_back(msl.get_decoration(a.id, spv::DecorationLocation));
            auto execution = stage == 0 ? spv::ExecutionModelVertex : spv::ExecutionModelFragment;
            spirv_cross::MSLResourceBinding ub{};
            ub.stage = execution;
            ub.desc_set = 0;
            ub.binding = 0;
            ub.msl_buffer = 0;
            msl.add_msl_resource_binding(ub);
            for (auto &s : activeResources.sampled_images) {
                unsigned binding = msl.get_decoration(s.id, spv::DecorationBinding);
                spirv_cross::MSLResourceBinding b{};
                b.stage = execution;
                b.desc_set = 1;
                b.binding = binding;
                b.msl_texture = binding;
                b.msl_sampler = binding;
                msl.add_msl_resource_binding(b);
                out.samplers.push_back(binding);
            }
            auto common = msl.get_common_options();
            common.vertex.fixup_clipspace = true;
            common.vertex.flip_vert_y = true;
            msl.set_common_options(common);
            auto opts = msl.get_msl_options();
            opts.set_msl_version(2, 3);
            opts.enable_decoration_binding = true;
            msl.set_msl_options(opts);
            msl.rename_entry_point("main", stage == 0 ? "etqw_vertex" : "etqw_fragment", execution);
            out.source = msl.compile();
        }
        return true;
    } catch (const std::exception &e) {
        error = e.what();
        return false;
    }
}
