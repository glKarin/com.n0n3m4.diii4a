#!/usr/bin/env python3
"""Exercise the actual blend parser without linking the game and its assets."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NEO = ROOT / 'doom3/neo'
source = (NEO / 'splashdamage/decllib/declRenderProgram.cpp').read_text()
start = source.index('void sdDeclRenderProgram::ParseBlend(')
end = source.index('void sdDeclRenderProgram::ParseDepthFunc(', start)
method = source[start:end]
constants = '\n'.join(re.findall(r'^const int GLS_.*?;', (NEO / 'renderer/tr_local.h').read_text(), re.M))
shim = r'''
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <strings.h>
struct idToken : std::string {
    int Icmp(const char* s) const { return strcasecmp(c_str(), s); }
};
struct idParser {
    std::istringstream tokens;
    idParser(const char* s) : tokens(s) {}
    bool ReadToken(idToken* t) { return bool(tokens >> *t); }
    bool ExpectTokenString(const char* s) { idToken t; return ReadToken(&t) && t == s; }
    void UnreadToken(idToken*) { assert(false); }
    void SkipRestOfLine() { assert(false); }
};
struct sdDeclRenderProgram {
    int drawStateBits;
    void ParseBlend(idParser&);
    int NameToSrcBlendMode(const idToken& t) {
        assert(t == "GL_SRC_ALPHA"); return GLS_SRCBLEND_SRC_ALPHA;
    }
    int NameToDstBlendMode(const idToken& t) {
        assert(t == "GL_ONE_MINUS_SRC_ALPHA"); return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    }
};
'''
tests = r'''
int main() {
    const char* modes[] = { "blend", "add", "filter", "modulate", "none",
        "GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA" };
    const int expected[] = {
        GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA,
        GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE,
        GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO,
        GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO,
        GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE,
        GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA };
    const int kept = GLS_DEPTHFUNC_EQUAL | GLS_DEPTHMASK | GLS_ALPHAMASK;
    for (int i = 0; i < 6; ++i) {
        sdDeclRenderProgram program{kept | GLS_SRCBLEND_DST_ALPHA | GLS_DSTBLEND_DST_ALPHA};
        idParser parser(modes[i]);
        program.ParseBlend(parser);
        if (program.drawStateBits != (kept | expected[i])) {
            std::cerr << "FAIL: " << modes[i] << " overwrote non-blend state or retained old factors\n";
            return 1;
        }
    }
    std::cout << "PASS: all 6 blend forms preserve depth equality and write masks, replacing old factors\n";
}
'''
probe = ROOT / 'build-macos-tests/render_state_probe.cpp'
probe.parent.mkdir(parents=True, exist_ok=True)
probe.write_text(constants + '\n' + shim + method + tests)
binary = probe.with_suffix('')
subprocess.run(['clang++', '-std=c++17', str(probe), '-o', str(binary)], check=True)
result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
print(result.stdout, end='')
