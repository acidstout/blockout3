/*
  File:        Hud.cpp
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
#include "Hud.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "Scene.h"

namespace bo3 {

static uint32_t Rgb(int r,int g,int b,int a = 255) {
  return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

// EGA style palette
static const uint32_t FRAME   = Rgb(85,85,255);
static const uint32_t LABEL   = Rgb(170,170,170);
static const uint32_t VALUE   = Rgb(255,255,85);
static const uint32_t GREEN   = Rgb(0,170,0);
static const uint32_t RED     = Rgb(255,85,85);
static const uint32_t WHITE   = Rgb(255,255,255);
static const uint32_t BLACK   = Rgb(0,0,0);

//-----------------------------------------------------------------------------
// UiBuilder

void UiBuilder::Quad(const float p[8],uint32_t c) {

  // Solid fill: sample the middle of the solid glyph (character 127)
  float u = (15 * font.glyphW + font.glyphW * 0.5f) / font.atlasW;
  float v = (5 * font.glyphH + font.glyphH * 0.5f) / font.atlasH;
  static const int idx[6] = { 0,1,2, 0,2,3 };
  for(int i : idx) Vtx(p[i*2],p[i*2+1],u,v,c);

}

void UiBuilder::Rect(float x,float y,float w,float h,uint32_t c) {
  const float p[8] = { x,y, x+w,y, x+w,y+h, x,y+h };
  Quad(p,c);
}

void UiBuilder::Frame(float x,float y,float w,float h,float t,uint32_t c) {
  Rect(x,y,w,t,c);
  Rect(x,y+h-t,w,t,c);
  Rect(x,y+t,t,h-2*t,c);
  Rect(x+w-t,y+t,t,h-2*t,c);
}

float UiBuilder::TextWidth(int scale,const char *s) const {
  return (float)(strlen(s) * font.glyphW * scale);
}

float UiBuilder::Text(float x,float y,int scale,const char *s,uint32_t c,int scaleY) {

  // Whole pixels only: the glyphs stay sharp
  x = floorf(x); y = floorf(y);
  float gw = (float)(font.glyphW * scale), gh = (float)(font.glyphH * (scaleY ? scaleY : scale));
  float x0 = x;
  for(; *s; s++) {
    int ch = (unsigned char)*s;
    if( ch<32 || ch>126 ) ch = '?';
    int col = (ch-32) % 16, row = (ch-32) / 16;
    float u0 = (float)(col*font.glyphW) / font.atlasW, u1 = (float)((col+1)*font.glyphW) / font.atlasW;
    float v0 = (float)(row*font.glyphH) / font.atlasH, v1 = (float)((row+1)*font.glyphH) / font.atlasH;
    if( ch!=' ' ) {
      Vtx(x,y,u0,v0,c);       Vtx(x+gw,y,u1,v0,c);    Vtx(x+gw,y+gh,u1,v1,c);
      Vtx(x,y,u0,v0,c);       Vtx(x+gw,y+gh,u1,v1,c); Vtx(x,y+gh,u0,v1,c);
    }
    x += gw;
  }
  return x - x0;

}

//-----------------------------------------------------------------------------

namespace {

struct Box { float x, y, w, h; };

// Largest integer scale for which text fits the box (with a margin)
int FitScale(const UiBuilder &ui,const Box &b,const char *s,float fill) {
  int byH = (int)(b.h * fill / ui.GlyphH());
  int byW = (int)(b.w * 0.9f / std::max(1.0f,ui.TextWidth(1,s)));
  return std::max(1,std::min(byH,byW));
}

void Centered(UiBuilder &ui,const Box &b,int scale,const char *s,uint32_t c) {
  ui.Text(b.x + (b.w - ui.TextWidth(scale,s))/2, b.y + (b.h - ui.GlyphH()*scale)/2, scale, s, c);
}

void RightAligned(UiBuilder &ui,const Box &b,int scale,const char *s,uint32_t c) {
  float margin = b.h * 0.2f;
  ui.Text(b.x + b.w - margin - ui.TextWidth(scale,s), b.y + (b.h - ui.GlyphH()*scale)/2, scale, s, c);
}

uint32_t LevelColor(int depth,int z) {
  Material m[MAT_COUNT];
  GetMaterials(m,0);
  const float *a = m[MAT_LEVEL0 + (depth-z-1) % 7].ambient;
  return Rgb((int)(a[0]*255),(int)(a[1]*255),(int)(a[2]*255));
}

uint32_t Scaled(uint32_t c,float f) {
  int r = (int)((c & 0xFF)*f), g = (int)(((c>>8) & 0xFF)*f), b = (int)(((c>>16) & 0xFF)*f);
  return Rgb(std::min(r,255),std::min(g,255),std::min(b,255));
}

// Column of the pit layers at the left (BlockOut II Pit::RenderLevel)
void PitLevel(UiBuilder &ui,const Pit &pit,const Box &b,float t) {

  int depth = pit.GetDepth();
  float slot = floorf((b.h - 2*t) / 20.0f);
  float top = b.y + b.h - slot*depth - 2*t;
  float inner = b.w - 4*t;

  ui.Rect(b.x,top,t,b.y+b.h-top,GREEN);
  ui.Rect(b.x+b.w-t,top,t,b.y+b.h-top,GREEN);
  ui.Rect(b.x,b.y+b.h-t,b.w,t,GREEN);

  for(int j=0;j<depth;j++) {
    float y = top + j*slot;
    ui.Rect(b.x,y,2*t,t,GREEN);
    ui.Rect(b.x+b.w-2*t,y,2*t,t,GREEN);
    if( pit.IsLineEmpty(j) ) continue;

    // Small cube: front, top and side faces
    uint32_t c = LevelColor(depth,j);
    float h = slot - 3*t;
    float d = floorf(h * 0.4f);                     // depth offset of the back face
    float x0 = b.x + 2*t, y0 = y + 2*t + d;
    float w = inner - d, fh = h - d;
    ui.Rect(x0,y0,w,fh,c);
    const float topFace[8]  = { x0,y0, x0+d,y0-d, x0+w+d,y0-d, x0+w,y0 };
    const float sideFace[8] = { x0+w,y0, x0+w+d,y0-d, x0+w+d,y0-d+fh, x0+w,y0+fh };
    ui.Quad(topFace,Scaled(c,1.35f));
    ui.Quad(sideFace,Scaled(c,0.6f));
  }

}

// Little star where a blocked rotation hit (replaces the spark sprite)
void Spark(UiBuilder &ui,float cx,float cy,float size,float f) {

  float alpha = 1.0f - f;
  uint32_t core = Rgb(255,255,170,(int)(255*alpha));
  uint32_t ray  = Rgb(255,170,0,(int)(255*alpha));
  float r0 = size * 0.15f * (1.0f + f), r1 = size * (0.35f + 0.65f*f);
  float th = std::max(1.0f,size * 0.06f * (1.0f - 0.5f*f));
  for(int i=0;i<8;i++) {
    float a = i * PI / 4.0f + f;
    float cs = cosf(a), sn = sinf(a), px = -sn*th, py = cs*th;
    const float p[8] = { cx+cs*r0+px, cy+sn*r0+py, cx+cs*r1+px, cy+sn*r1+py,
                         cx+cs*r1-px, cy+sn*r1-py, cx+cs*r0-px, cy+sn*r0-py };
    ui.Quad(p,ray);
  }
  float c = size * 0.12f * (1.0f - f);
  ui.Rect(cx-c,cy-c,2*c,2*c,core);

}

} // namespace

//-----------------------------------------------------------------------------

void BuildHud(const GameLogic &game,int canvasW,int canvasH,const FontInfo &font,std::vector<UiVertex> &out) {

  UiBuilder ui(out,font);
  const float W = (float)canvasW, H = (float)canvasH;
  // Layout in BlockOut II 1024x768 screen units, snapped to whole pixels
  auto box = [&](float x,float y,float w,float h) {
    float x0 = floorf(x*W/1024.0f), y0 = floorf(y*H/768.0f);
    return Box{ x0, y0, floorf((x+w)*W/1024.0f) - x0, floorf((y+h)*H/768.0f) - y0 };
  };
  const float t = std::max(1.0f,floorf(H/384.0f));          // line thickness
  // Text scale: the longest label ("CUBES PLAYED") fills most of the right column
  const int   s = std::max(1,(int)(W*176.0f/1024.0f*0.95f / (12.0f*font.glyphW)));
  const Pit  &pit = game.GetPit();
  const SCOREREC &score = game.GetScore();
  char tmp[64];

  // Pit frame
  Box pv = { (float)fround(W*0.0889f), (float)fround(H*0.0183f), (float)fround(W*0.7197f), (float)fround(H*0.9596f) };
  ui.Frame(pv.x-2*t,pv.y-2*t,pv.w+4*t,pv.h+4*t,t,FRAME);

  // Level: number and pit layers
  Box lv = box(16,47,56,53);
  Centered(ui,box(10,16,68,26),s,"LEVEL",LABEL);
  ui.Frame(lv.x,lv.y,lv.w,lv.h,t,FRAME);
  snprintf(tmp,sizeof(tmp),"%d",game.GetLevel());
  Centered(ui,lv,FitScale(ui,lv,"10",0.7f),tmp,VALUE);
  Box col = box(16,114,56,638);
  ui.Frame(col.x-t,col.y-t,col.w+2*t,col.h+2*t,t,FRAME);
  PitLevel(ui,pit,col,t);

  // Logo
  Box logo = box(852,26,154,104);
  ui.Frame(logo.x,logo.y,logo.w,logo.h,t,FRAME);
  int ls = FitScale(ui,Box{ logo.x,logo.y,logo.w,logo.h/3 },"BLOCK",0.8f);
  float lh = (float)font.glyphH*ls;
  float ly = logo.y + (logo.h - 3*lh - 2*t)/2;
  Centered(ui,Box{ logo.x,ly,logo.w,lh },ls,"BLOCK",RED);
  Centered(ui,Box{ logo.x,ly+lh+t,logo.w,lh },ls,"OUT",FRAME);
  Centered(ui,Box{ logo.x,ly+2*lh+2*t,logo.w,lh },ls,"III",WHITE);

  // Right column: label above a value box
  auto field = [&](float labelY,float boxY,const char *label,const char *value) {
    Centered(ui,box(840,labelY-13,176,26),s,label,LABEL);
    Box b = box(852,boxY,154,44);
    ui.Frame(b.x,b.y,b.w,b.h,t,FRAME);
    RightAligned(ui,b,std::max(1,std::min(FitScale(ui,b,"0000000",0.55f),s+1)),value,VALUE);
  };
  snprintf(tmp,sizeof(tmp),"%d",score.score);
  field(175,188,"SCORE",tmp);
  snprintf(tmp,sizeof(tmp),"%d",score.nbCube);
  field(273,292,"CUBES PLAYED",tmp);
  snprintf(tmp,sizeof(tmp),"%d",std::max(game.GetHighScore(),score.score));
  field(484,502,"HIGH SCORE",tmp);
  snprintf(tmp,sizeof(tmp),"%dx%dx%d",pit.GetWidth(),pit.GetHeight(),pit.GetDepth());
  field(590,607,"PIT",tmp);
  static const char *setName[] = { "FLAT", "BASIC", "EXTENDED" };
  field(697,712,"BLOCK SET",setName[game.GetBlockSet()]);

  if( game.IsDemo() || game.IsPractice() ) {
    const char *mode = game.IsDemo() ? "DEMO" : "PRACTICE";
    Box b = box(850,390,158,35);
    Centered(ui,b,FitScale(ui,b,"PRACTICE",0.8f),mode,RED);
  }

  // Practice: help text during the first 3 seconds
  float now = game.GetCurrentTime();
  if( game.IsPractice() && now - game.GetGameStartTime() < 3.0f ) {
    const char *help = "Press [H] to see suggested position";
    int hs = FitScale(ui,pv,help,0.05f);
    Box hb = { pv.x, pv.y + (pv.h - font.glyphH*hs)/2, pv.w, (float)font.glyphH*hs };
    ui.Rect(hb.x + (hb.w - ui.TextWidth(hs,help))/2 - 4*t, hb.y - 2*t, ui.TextWidth(hs,help) + 8*t, hb.h + 4*t, Rgb(0,0,0,170));
    Centered(ui,hb,hs,help,WHITE);
  }

  // Pause and game over
  int mode = game.GetMode();
  if( mode==GAME_PAUSED || mode==GAME_OVER ) {
    const char *msg = mode==GAME_PAUSED ? "PAUSED" : "GAME OVER";
    Box b = box(333,320,256,129);
    ui.Rect(b.x,b.y,b.w,b.h,BLACK);
    ui.Frame(b.x,b.y,b.w,b.h,t,FRAME);
    ui.Frame(b.x+2*t,b.y+2*t,b.w-4*t,b.h-4*t,t,FRAME);
    Centered(ui,b,FitScale(ui,b,"GAME OVER",0.35f),msg,mode==GAME_PAUSED ? VALUE : RED);
  }

  // Spark of a blocked rotation (0.5 s)
  float sparkStart = game.GetSparkTime();
  if( sparkStart!=0.0f && mode!=GAME_PAUSED && mode!=GAME_OVER ) {
    float f = std::clamp((now - sparkStart) / 0.5f,0.0f,1.0f);
    PitGeometry g = MakePitGeometry(pit.GetWidth(),pit.GetHeight(),pit.GetDepth());
    BLOCKITEM p = game.GetSparkPos();
    Matrix m = Mul(Perspective(60.0f,1.0f,0.1f,10.0f),GameView());
    float rx,ry,rz,rw;
    m.TransfomVec((p.x+0.5f)*g.cubeSide + g.origin.x,(p.y+0.5f)*g.cubeSide + g.origin.y,
                  (p.z+0.5f)*g.cubeSide + g.origin.z,1.0f,&rx,&ry,&rz,&rw);
    float sx = pv.x + (rx/rw + 1.0f) * 0.5f * pv.w;
    float sy = pv.y + (1.0f - ry/rw) * 0.5f * pv.h;
    float size = W * 0.05f * ((1.0f - (float)p.z/pit.GetDepth())*0.33f + 0.66f);
    Spark(ui,sx,sy,size,f);
  }

}

} // namespace bo3
