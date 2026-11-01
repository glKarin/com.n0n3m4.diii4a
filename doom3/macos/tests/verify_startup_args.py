#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the production startup parser under ASan with long launcher arguments."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[3]
source = (root / "doom3/neo/framework/Common.cpp").read_text()
start = source.index("#define\t\tMAX_CONSOLE_LINES")
end = source.index("\n/*", source.index("void idCommonLocal::ParseCommandLine", start))
shim = r'''
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
struct idCmdArgs {
    std::vector<std::string> args;
    void AppendArg(const char* s) { args.emplace_back(s); }
};
struct idCommonLocal {
    void ParseCommandLine(int,const char**);
    void FatalError(const char*,...) { throw std::runtime_error("too many commands"); }
};
'''
test = r'''
int main() {
    idCommonLocal common;
    // Actual launchers combine renderer defaults, profile paths and server settings.
    for(int count:{0,1,32,33,72,MAX_CONSOLE_LINES,MAX_CONSOLE_LINES+1}) {
        for(auto& line:com_consoleLines) line.args.clear();
        std::vector<const char*> argv;
        for(int i=0;i<count;i++) { argv.push_back("+set");argv.push_back("name");argv.push_back("with spaces"); }
        bool rejected=false;
        try { common.ParseCommandLine(argv.size(),argv.data()); }
        catch(const std::runtime_error&) { rejected=true; }
        assert(rejected==(count>MAX_CONSOLE_LINES));
        if(!rejected) {
            assert(com_numConsoleLines==count);
            for(int i=0;i<count;i++) {
                assert(com_consoleLines[i].args.size()==3);
                assert(com_consoleLines[i].args[0]=="set");
                assert(com_consoleLines[i].args[2]=="with spaces");
            }
        }
    }
    puts("PASS: startup commands beyond 32 retained; 128-command bound rejected without overflow (ASan)");
}
'''
probe = root / "build-macos-tests/startup_args_probe.cpp"
probe.parent.mkdir(parents=True, exist_ok=True)
probe.write_text(shim + source[start:end] + test)
subprocess.run(["clang++", "-std=c++17", "-fsanitize=address", str(probe), "-o", str(probe.with_suffix(""))], check=True)
subprocess.run([str(probe.with_suffix(""))], check=True)
