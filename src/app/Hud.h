/*
  File:        Hud.h
  Description: 2D overlay of the game screen: frames, texts, pit level column,
               spark (no D3D)
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS (Sprites.cpp, Pit::RenderLevel)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#pragma once

#include <cstdint>
#include <vector>
#include "GameLogic.h"

namespace bo3 {

// Triangle list vertex in canvas pixels; uv into the font atlas
struct UiVertex {
  float    x, y;
  float    u, v;
  uint32_t color;   // 0xAABBGGRR
};

// Font atlas: characters 32..127 in 16 columns x 6 rows of glyph cells.
// Character 127 is a solid cell, used for filled shapes.
struct FontInfo {
  int glyphW, glyphH;    // cell size in atlas pixels
  int atlasW, atlasH;
};

class UiBuilder {

  public:
    UiBuilder(std::vector<UiVertex> &out,const FontInfo &font) : out(out), font(font) {}

    void Rect(float x,float y,float w,float h,uint32_t color);
    void Frame(float x,float y,float w,float h,float t,uint32_t color);
    void Quad(const float p[8],uint32_t color);   // 4 corners x,y
    // Text with integer pixel scale; returns its width
    // scaleY: vertical scale when different (0 = scale)
    float Text(float x,float y,int scale,const char *s,uint32_t color,int scaleY = 0);
    float TextWidth(int scale,const char *s) const;
    int   GlyphH() const { return font.glyphH; }

  private:
    void Vtx(float x,float y,float u,float v,uint32_t c) { out.push_back(UiVertex{ x,y,u,v,c }); }
    std::vector<UiVertex> &out;
    FontInfo font;
};

// Game screen HUD for a canvas of the given size (4:3)
void BuildHud(const GameLogic &game,int canvasW,int canvasH,const FontInfo &font,std::vector<UiVertex> &out);

} // namespace bo3
