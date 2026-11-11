#pragma once
#include <string>
#include <vector>

using namespace std;

struct TextBitmap {
    int width, height;
    vector<unsigned char> pixels;
};

void loadFont(const char* executable);
bool fontHasGlyph(int codepoint);
TextBitmap rasterizeText(const string& text);
