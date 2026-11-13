#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the production rectangle clipper and verify HUD fill geometry/UVs."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[3]
source = (root / "doom3/neo/splashdamage/renderer/DeviceContext.cpp").read_text()
start = source.index("bool sdDeviceContextLocal::ClippedCoords(")
end = source.index("\nvoid sdDeviceContextLocal::DrawStretchPic(", start)
shim = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
template<class T> T Min(T a,T b) { return std::min(a,b); }
template<class T> T Max(T a,T b) { return std::max(a,b); }
struct sdBounds2D {
    float x,y,w,h;
    float GetLeft() const { return x; } float GetRight() const { return x+w; }
    float GetTop() const { return y; } float GetBottom() const { return y+h; }
};
template<class T> struct idList:std::vector<T> { using std::vector<T>::operator=; int Num() const { return this->size(); } };
struct sdDeviceContextLocal {
    bool enableClipping=true;
    idList<sdBounds2D> clipRects;
    bool ClippedCoords(float*,float*,float*,float*,float*,float*,float*,float*);
};
void near(float a,float b) { assert(std::abs(a-b)<0.0001f); }
'''
tests = r'''
int main() {
    sdDeviceContextLocal dc;
    for(float fill:{0.f, .25f, .5f, 1.f}) {
        dc.clipRects={{100,50,200*fill,20}};
        float x=100,y=50,w=200,h=20,s=0,t=0,u=1,v=1;
        bool hidden=dc.ClippedCoords(&x,&y,&w,&h,&s,&t,&u,&v);
        assert(hidden==(fill==0)); near(w,200*fill);
        if(!hidden) { near(u,fill); near(s,0); near(h,20); }
    }
    // Both parent and child constrain the result, including stack entry zero.
    dc.clipRects={{110,55,80,10},{100,50,120,20}};
    float x=100,y=50,w=200,h=20,s=1,t=0,u=0,v=1;
    assert(!dc.ClippedCoords(&x,&y,&w,&h,&s,&t,&u,&v));
    near(x,110); near(y,55); near(w,80); near(h,10);
    near(s,.95f); near(u,.55f); near(t,.25f); near(v,.75f);
    // Fully disjoint and zero-size masks must not generate negative geometry.
    for(auto clip:{sdBounds2D{500,50,10,20},sdBounds2D{100,50,0,20}}) {
        dc.clipRects={clip}; x=100;y=50;w=200;h=20;
        assert(dc.ClippedCoords(&x,&y,&w,&h,nullptr,nullptr,nullptr,nullptr));
        near(w,0); near(h,0);
    }
    // Explicitly disabled clipping and a popped/empty stack retain full bars.
    dc.enableClipping=false; x=100;y=50;w=200;h=20;
    assert(!dc.ClippedCoords(&x,&y,&w,&h,nullptr,nullptr,nullptr,nullptr)); near(w,200);
    dc.enableClipping=true; dc.clipRects.clear();
    assert(!dc.ClippedCoords(&x,&y,&w,&h,nullptr,nullptr,nullptr,nullptr)); near(w,200);
    puts("PASS: HUD bars at 0/25/50/100%, nested masks, flipped UVs, empty/outside masks and reset");
}
'''
probe = root / "build-macos-tests/hud_bars_probe.cpp"
probe.parent.mkdir(parents=True, exist_ok=True)
probe.write_text(shim + source[start:end] + tests)
subprocess.run(["clang++", "-std=c++17", str(probe), "-o", str(probe.with_suffix(""))], check=True)
subprocess.run([str(probe.with_suffix(""))], check=True)
