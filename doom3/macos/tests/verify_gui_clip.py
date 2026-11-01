#!/usr/bin/env python3
"""Compile the real GUI clipping methods and check geometry/UV invariants."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[3]
source = (root / 'doom3/neo/renderer/GuiModel.cpp').read_text()
start = source.index('void sdGuiModel::SetClipShape(')
end = source.index('\n/*', start)
methods = source[start:end]
shim = r'''
#include <vector>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <algorithm>
using glIndex_t = unsigned;
struct idVec3 { float x=0,y=0,z=0; idVec3()=default; idVec3(float a,float b,float c):x(a),y(b),z(c){} };
struct idVec4 { float x,y,z,w; };
struct UV { float x=0,y=0; };
struct idDrawVert {
    idVec3 xyz; UV st;
    void Lerp(const idDrawVert& a,const idDrawVert& b,float f) {
        xyz={a.xyz.x+(b.xyz.x-a.xyz.x)*f,a.xyz.y+(b.xyz.y-a.xyz.y)*f,0};
        st={a.st.x+(b.st.x-a.st.x)*f,a.st.y+(b.st.y-a.st.y)*f};
    }
    void LerpAll(const idDrawVert& a,const idDrawVert& b,float f) { Lerp(a,b,f); }
};
template<class T> struct idList:std::vector<T> {
    int Num() const { return this->size(); }
    void Clear() { this->clear(); }
    void SetNum(int n,bool=false) { this->resize(n); }
    void Append(const T& v) { this->push_back(v); }
    void Swap(idList& v) { this->swap(v); }
};
struct idMath { static constexpr float PI=3.14159265358979323846f,TWO_PI=2*PI;
    static float Cos(float a) { return std::cos(a); } static float Sin(float a) { return std::sin(a); } };
struct sdGuiModel {
    idList<idVec3> clipShapePlanes;
    void SetClipShape(const idVec4*,bool);
    void ClipShapeGeometry(const idDrawVert*,const glIndex_t*,int,idList<idDrawVert>&,idList<glIndex_t>&,bool) const;
};
'''
tests = r'''
double area(const idList<idDrawVert>& v,const idList<glIndex_t>& idx) {
    double result=0;
    for(size_t i=0;i<idx.size();i+=3) {
        auto a=v[idx[i]].xyz,b=v[idx[i+1]].xyz,c=v[idx[i+2]].xyz;
        result+=std::abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5;
    } return result;
}
int main() {
    sdGuiModel model;
    const glIndex_t triangles[]={0,1,2,0,2,3};
    // Oversized rotated terrain overlay spanning the whole HUD.
    idDrawVert input[4];
    const float xy[4][2]={{-1000,200},{200,-1000},{1400,200},{200,1400}};
    for(int i=0;i<4;++i) { input[i].xyz={xy[i][0],xy[i][1],0};input[i].st={xy[i][0]*.01f,xy[i][1]*.01f}; }
    const idVec4 rect={400,50,180,100};
    for(bool ellipse:{false,true}) {
        model.SetClipShape(&rect,ellipse);
        idList<idDrawVert> out;idList<glIndex_t> indices;
        model.ClipShapeGeometry(input,triangles,6,out,indices,false);
        assert(indices.Num()>0 && indices.Num()%3==0);
        for(const auto& v:out) {
            assert(v.xyz.x>=399.999f && v.xyz.x<=580.001f && v.xyz.y>=49.999f && v.xyz.y<=150.001f);
            assert(std::abs(v.st.x-v.xyz.x*.01f)<.0001f && std::abs(v.st.y-v.xyz.y*.01f)<.0001f);
            if(ellipse) assert(std::pow((v.xyz.x-490)/90,2)+std::pow((v.xyz.y-100)/50,2)<=1.00002f);
        }
        double expected=ellipse?90*50*idMath::PI:180*100;
        assert(std::abs(area(out,indices)/expected-1)<.002);
    }
    model.SetClipShape(nullptr,false);
    assert(model.clipShapePlanes.Num()==0);
    const idVec4 empty={400,50,0,100};model.SetClipShape(&empty,true);
    idList<idDrawVert> out;idList<glIndex_t> indices;
    model.ClipShapeGeometry(input,triangles,6,out,indices,true);
    assert(indices.Num()==0);
    puts("PASS: rotated overlays stay inside rectangle/ellipse, UVs and area preserved; empty/reset masks work");
}
'''
probe = root / 'build-macos-tests/gui_clip_probe.cpp'
probe.parent.mkdir(parents=True, exist_ok=True)
probe.write_text(shim + methods + tests)
binary = probe.with_suffix('')
subprocess.run(['clang++', '-std=c++17', str(probe), '-o', str(binary)], check=True)
result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
print(result.stdout, end='')
