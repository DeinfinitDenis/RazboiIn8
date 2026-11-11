#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "text_renderer.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
vector<unsigned char> fontBytes;
stbtt_fontinfo fontInfo{};

filesystem::path executableDirectory(const char* executable) {
#ifdef _WIN32
    vector<wchar_t> path(32768);
    DWORD length=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    if(length>0 && length<path.size()) return filesystem::path(path.data()).parent_path();
#else
    error_code error;
    auto path=filesystem::read_symlink("/proc/self/exe",error);
    if(!error) return path.parent_path();
#endif
    return filesystem::absolute(executable).parent_path();
}

vector<int> codepoints(const string& text) {
    vector<int> result;
    for(size_t i=0;i<text.size();) {
        unsigned char first=static_cast<unsigned char>(text[i++]);
        int value=first, remaining=0;
        if(first>=0xc2 && first<=0xdf) { value=first&0x1f; remaining=1; }
        else if(first>=0xe0 && first<=0xef) { value=first&0x0f; remaining=2; }
        else if(first>=0xf0 && first<=0xf4) { value=first&7; remaining=3; }
        else if(first>=0x80) value='?';
        while(remaining>0 && i<text.size() && (static_cast<unsigned char>(text[i])&0xc0)==0x80) {
            value=(value<<6)|(static_cast<unsigned char>(text[i++])&0x3f);
            --remaining;
        }
        result.push_back(remaining>0?'?':value);
    }
    return result;
}
}

void loadFont(const char* executable) {
    auto path=executableDirectory(executable)/"assets"/"NotoSans-Regular.ttf";
    ifstream input(path,ios::binary);
    if(!input) throw runtime_error("Nu se poate deschide fontul: "+path.string());
    fontBytes.assign(istreambuf_iterator<char>(input),istreambuf_iterator<char>());
    if(fontBytes.size()<12 || !stbtt_InitFont(&fontInfo,fontBytes.data(),0))
        throw runtime_error("Font TrueType invalid: "+path.string());
}

bool fontHasGlyph(int codepoint) {
    return stbtt_FindGlyphIndex(&fontInfo,codepoint)!=0;
}

TextBitmap rasterizeText(const string& text) {
    constexpr int height=48, padding=2;
    float scale=stbtt_ScaleForPixelHeight(&fontInfo,float(height-padding*2));
    int ascent,descent,lineGap;
    stbtt_GetFontVMetrics(&fontInfo,&ascent,&descent,&lineGap);
    int baseline=padding+int(ceil(ascent*scale));
    auto codes=codepoints(text);
    struct Glyph { int code,x,y,width,height; };
    vector<Glyph> glyphs;
    float cursor=padding;
    int left=0,right=padding;
    for(size_t i=0;i<codes.size();++i) {
        int code=fontHasGlyph(codes[i])?codes[i]:'?';
        int x0,y0,x1,y1,advance,bearing;
        stbtt_GetCodepointBitmapBox(&fontInfo,code,scale,scale,&x0,&y0,&x1,&y1);
        int x=int(round(cursor))+x0;
        glyphs.push_back({code,x,baseline+y0,x1-x0,y1-y0});
        left=min(left,x); right=max(right,x+x1-x0);
        stbtt_GetCodepointHMetrics(&fontInfo,code,&advance,&bearing);
        cursor+=advance*scale;
        if(i+1<codes.size()) cursor+=stbtt_GetCodepointKernAdvance(&fontInfo,code,codes[i+1])*scale;
    }
    int width=max(right,int(ceil(cursor)))-left+padding;
    TextBitmap bitmap{width,height,vector<unsigned char>(width*height,0)};
    for(auto glyph:glyphs) {
        if(!glyph.width || !glyph.height) continue;
        vector<unsigned char> pixels(glyph.width*glyph.height);
        stbtt_MakeCodepointBitmap(&fontInfo,pixels.data(),glyph.width,glyph.height,glyph.width,scale,scale,glyph.code);
        for(int y=0;y<glyph.height;++y) for(int x=0;x<glyph.width;++x) {
            int targetY=glyph.y+y,targetX=glyph.x-left+x;
            if(targetY>=0 && targetY<height) {
                auto& target=bitmap.pixels[targetY*width+targetX];
                unsigned alpha=pixels[y*glyph.width+x];
                target=static_cast<unsigned char>(alpha+target*(255-alpha)/255);
            }
        }
    }
    return bitmap;
}
