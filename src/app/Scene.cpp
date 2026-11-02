/*
  File:        Scene.cpp
  Description: 3D geometry of the pit and the falling block (no D3D)
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS (Pit.cpp, PolyCube.cpp, Game.cpp)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "Scene.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace bo3 {

static const float STARTZ = 0.87f;

//-----------------------------------------------------------------------------
// Primitive helpers

static uint32_t Info(MaterialId m,VertexKind k,int end = 0,int side = 0) {
  return (uint32_t)m | ((uint32_t)k << 8) | ((uint32_t)end << 12) | ((uint32_t)side << 13);
}

static void Tri(std::vector<Vertex> &out,const VERTEX &p,const VERTEX &n,uint32_t info) {
  out.push_back(Vertex{ {p.x,p.y,p.z}, {n.x,n.y,n.z}, info });
}

// GL_QUADS quad (a,b,c,d) as two triangles with the same winding
static void Quad(std::vector<Vertex> &out,const VERTEX q[4],const VERTEX n[4],MaterialId m,VertexKind k) {
  static const int idx[6] = { 0,1,2, 0,2,3 };
  for(int i : idx) Tri(out,q[i],n[i],Info(m,k));
}

static void Quad(std::vector<Vertex> &out,const VERTEX q[4],const VERTEX &n,MaterialId m,VertexKind k) {
  const VERTEX ns[4] = { n,n,n,n };
  Quad(out,q,ns,m,k);
}

static void Line(std::vector<Vertex> &out,const VERTEX &a,const VERTEX &b,MaterialId m) {
  // end, side (1 = +perpendicular) of the 6 vertices of the screen quad
  static const int corner[6][2] = { {0,1},{0,0},{1,1}, {1,1},{0,0},{1,0} };
  for(const auto &c : corner) {
    const VERTEX &p = c[0] ? b : a;
    const VERTEX &q = c[0] ? a : b;
    out.push_back(Vertex{ {p.x,p.y,p.z}, {q.x,q.y,q.z}, Info(m,VK_LINE,c[0],c[1]) });
  }
}

static VERTEX Add(const VERTEX &a,float x,float y,float z) { return v(a.x+x,a.y+y,a.z+z); }

//-----------------------------------------------------------------------------

void GetMaterials(Material out[MAT_COUNT],int transparency) {

  memset(out,0,sizeof(Material)*MAT_COUNT);
  auto set = [&](MaterialId id,float ar,float ag,float ab,float dr,float dg,float db) {
    Material &m = out[id];
    m.ambient[0] = ar; m.ambient[1] = ag; m.ambient[2] = ab;
    m.diffuse[0] = dr; m.diffuse[1] = dg; m.diffuse[2] = db;
  };
  set(MAT_GRID, 0.0f,0.6f,0.0f, 1.0f,1.0f,1.0f);
  set(MAT_BACK, 0.05f,0.05f,0.1f, 0.05f,0.05f,0.1f);
  set(MAT_WHITE,1.0f,1.0f,1.0f, 1.0f,1.0f,1.0f);
  set(MAT_RED,  1.0f,0.0f,0.0f, 1.0f,0.0f,0.0f);
  set(MAT_GRAY, 0.5f,0.5f,0.5f, 0.8f,0.8f,0.8f);
  out[MAT_GRAY].specular[0] = out[MAT_GRAY].specular[1] = out[MAT_GRAY].specular[2] = 1.0f;
  out[MAT_GRAY].specular[3] = 20.0f;

  // Transparent faces of the falling block
  float trans = ((float)transparency / 10.0f) * 0.4f;
  set(MAT_GHOST,0.5f,0.5f,0.5f, 0.5f,0.5f,0.5f);
  out[MAT_GHOST].ambient[3] = out[MAT_GHOST].diffuse[3] = trans;

  // Menu grid (BlockOut II MenuGrid)
  set(MAT_MENU_GRID,0.0f,0.5f,0.0f, 1.0f,1.0f,1.0f);
  set(MAT_MENU_RED, 1.0f,0.0f,0.0f, 0.5f,0.0f,0.0f);
  set(MAT_MENU_BLUE,0.2f,0.2f,0.7f, 0.0f,0.0f,1.0f);

  // Pit layer colours (BlockOut II Pit::GetMaterial)
  static const float level[7][3] = { {0.0f,0.0f,1.0f}, {0.0f,1.0f,0.0f}, {0.0f,0.9f,0.9f},
                                     {1.0f,0.0f,0.0f}, {1.0f,0.1f,0.8f}, {0.9f,0.6f,0.0f},
                                     {0.85f,0.85f,0.85f} };
  for(int i=0;i<7;i++) {
    const float *c = level[i];
    set((MaterialId)(MAT_LEVEL0+i), c[0]*0.7f,c[1]*0.7f,c[2]*0.7f, c[0]*0.3f,c[1]*0.3f,c[2]*0.3f);
  }

}

//-----------------------------------------------------------------------------

PitGeometry MakePitGeometry(int width,int height,int depth) {

  PitGeometry g;
  g.width = width; g.height = height; g.depth = depth;
  if( width>height ) {
    g.cubeSide = 1.0f / (float)width;
    g.origin = v( -0.5f, -0.5f + ( (float)(width-height)*g.cubeSide/2.0f ), STARTZ );
  } else {
    g.cubeSide = 1.0f / (float)height;
    g.origin = v( -0.5f + ( (float)(height-width)*g.cubeSide/2.0f ), -0.5f, STARTZ );
  }
  g.fWidth  = (float)width  * g.cubeSide;
  g.fHeight = (float)height * g.cubeSide;
  g.fDepth  = (float)depth  * g.cubeSide;
  return g;

}

//-----------------------------------------------------------------------------
// Pit

// Unit cube edges (BlockOut II lcubeList)
static const int cubeEdge[12][2][3] = {
  {{0,1,0},{1,1,0}}, {{1,1,0},{1,0,0}}, {{1,0,0},{0,0,0}}, {{0,0,0},{0,1,0}},
  {{0,1,0},{0,1,1}}, {{1,1,0},{1,1,1}}, {{1,0,0},{1,0,1}}, {{0,0,0},{0,0,1}},
  {{0,1,1},{1,1,1}}, {{1,1,1},{1,0,1}}, {{0,0,1},{1,0,1}}, {{0,0,1},{0,1,1}} };

static void PitWalls(const PitGeometry &g,std::vector<Vertex> &out) {

  const VERTEX o = g.origin;
  const float W = g.fWidth, H = g.fHeight, D = g.fDepth, c = g.cubeSide;

  // Side: black frame around the pit opening (unlit)
  const VERTEX side[24] = {
    Add(o,W,H,0), Add(o,-c,H+c,0), Add(o,W+c,H+c,0),
    Add(o,W,H,0), Add(o,0,H,0),    Add(o,-c,H+c,0),
    Add(o,0,H,0), Add(o,-c,-c,0),  Add(o,-c,H+c,0),
    Add(o,0,H,0), Add(o,0,0,0),    Add(o,-c,-c,0),
    Add(o,0,0,0), Add(o,W+c,-c,0), Add(o,-c,-c,0),
    Add(o,0,0,0), Add(o,W,0,0),    Add(o,W+c,-c,0),
    Add(o,W+c,-c,0), Add(o,W,0,0), Add(o,W,H,0),
    Add(o,W+c,-c,0), Add(o,W,H,0), Add(o,W+c,H+c,0) };
  for(const VERTEX &p : side) Tri(out,p,v(0,0,0),Info(MAT_BLACK,VK_UNLIT));

  // Back: the 4 walls and the bottom (lit)
  const VERTEX f1[4] = { Add(o,0,H,0), Add(o,0,H,D), Add(o,0,0,D), Add(o,0,0,0) };
  const VERTEX f2[4] = { Add(o,0,H,0), Add(o,W,H,0), Add(o,W,H,D), Add(o,0,H,D) };
  const VERTEX f3[4] = { Add(o,W,H,D), Add(o,W,H,0), Add(o,W,0,0), Add(o,W,0,D) };
  const VERTEX f4[4] = { Add(o,W,0,0), Add(o,0,0,0), Add(o,0,0,D), Add(o,W,0,D) };
  const VERTEX f5[4] = { Add(o,0,H,D), Add(o,W,H,D), Add(o,W,0,D), Add(o,0,0,D) };
  Quad(out,f1,v( 1, 0, 0),MAT_BACK,VK_LIT);
  Quad(out,f2,v( 0,-1, 0),MAT_BACK,VK_LIT);
  Quad(out,f3,v(-1, 0, 0),MAT_BACK,VK_LIT);
  Quad(out,f4,v( 0, 1, 0),MAT_BACK,VK_LIT);
  Quad(out,f5,v( 0, 0,-1),MAT_BACK,VK_LIT);

}

static void PitGrid(const PitGeometry &g,std::vector<Vertex> &out) {

  const VERTEX o = g.origin;
  const float c = g.cubeSide, D = g.fDepth;

  for(int i=0;i<=g.width;i++) {
    Line(out,v(o.x+i*c,-o.y,o.z),v(o.x+i*c,-o.y,o.z+D),MAT_GRID);
    Line(out,v(o.x+i*c, o.y,o.z),v(o.x+i*c, o.y,o.z+D),MAT_GRID);
  }
  for(int i=1;i<g.height;i++) {
    Line(out,v( o.x,o.y+i*c,o.z),v( o.x,o.y+i*c,o.z+D),MAT_GRID);
    Line(out,v(-o.x,o.y+i*c,o.z),v(-o.x,o.y+i*c,o.z+D),MAT_GRID);
  }
  for(int i=0;i<=g.depth;i++) {
    float z = o.z + i*c;
    Line(out,v( o.x,-o.y,z),v(-o.x,-o.y,z),MAT_GRID);
    Line(out,v(-o.x,-o.y,z),v(-o.x, o.y,z),MAT_GRID);
    Line(out,v(-o.x, o.y,z),v( o.x, o.y,z),MAT_GRID);
    Line(out,v( o.x, o.y,z),v( o.x,-o.y,z),MAT_GRID);
  }
  for(int i=1;i<g.width;i++)
    Line(out,v(o.x+i*c,-o.y,o.z+D),v(o.x+i*c,o.y,o.z+D),MAT_GRID);
  for(int i=1;i<g.height;i++)
    Line(out,v(o.x,o.y+i*c,o.z+D),v(-o.x,o.y+i*c,o.z+D),MAT_GRID);

}

// Edges in green at the pit border, black inside (BlockOut II Pit::RenderEdge)
static bool IsGreenEdge(const PitGeometry &g,int x,int y,int z,int edge) {

  switch(edge) {
    case 0:  return y==g.height-1;
    case 1:  return x==g.width-1;
    case 2:  return y==0;
    case 3:  return x==0;
    case 4:  return x==0 || y==g.height-1;
    case 5:  return x==g.width-1 || y==g.height-1;
    case 6:  return x==g.width-1 || y==0;
    case 7:  return x==0 || y==0;
    default: return z==g.depth-1;
  }

}

static void PitCube(const PitGeometry &g,int x,int y,int z,std::vector<Vertex> &out) {

  const float c = g.cubeSide;
  const VERTEX o = Add(g.origin,x*c,y*c,z*c);
  MaterialId m = (MaterialId)(MAT_LEVEL0 + (g.depth-z-1) % 7);

  // Faces (unlit, BlockOut II Pit::CreateFillingCube)
  const VERTEX faces[6][4] = {
    { Add(o,0,c,0), Add(o,c,c,0), Add(o,c,0,0), Add(o,0,0,0) },
    { Add(o,c,c,0), Add(o,c,c,c), Add(o,c,0,c), Add(o,c,0,0) },
    { Add(o,c,c,c), Add(o,0,c,c), Add(o,0,0,c), Add(o,c,0,c) },
    { Add(o,0,c,c), Add(o,0,c,0), Add(o,0,0,0), Add(o,0,0,c) },
    { Add(o,c,0,0), Add(o,c,0,c), Add(o,0,0,c), Add(o,0,0,0) },
    { Add(o,0,c,c), Add(o,c,c,c), Add(o,c,c,0), Add(o,0,c,0) } };
  for(const auto &f : faces) Quad(out,f,v(0,0,0),m,VK_UNLIT);

  // Outline: front square, then the sides facing the pit centre
  auto edge = [&](int e) {
    const int (*p)[3] = cubeEdge[e];
    Line(out,Add(o,p[0][0]*c,p[0][1]*c,p[0][2]*c),Add(o,p[1][0]*c,p[1][1]*c,p[1][2]*c),
         IsGreenEdge(g,x,y,z,e) ? MAT_GRID : MAT_BLACK);
  };
  int mX = g.width/2;
  int mY = g.height/2;
  edge(0); edge(1); edge(2); edge(3);
  if( x<mX ) { edge(5); edge(9); edge(6); }
  if( x>mX ) { edge(4); edge(11); edge(7); }
  if( y<mY ) { edge(4); edge(8); edge(5); }
  if( y>mY ) { edge(7); edge(10); edge(6); }

}

void BuildPit(const Pit &pit,const PitGeometry &g,bool renderCubes,bool zBuffer,std::vector<Vertex> &out) {

  if( !zBuffer ) PitWalls(g,out);
  PitGrid(g,out);
  if( !renderCubes ) return;

  // Far cubes first: decreasing distance to the top centre of the pit
  struct Item { int x,y,z; double d; };
  std::vector<Item> order;
  double xOrg = g.width/2.0, yOrg = g.height/2.0;
  for(int k=0;k<g.depth;k++)
    for(int j=0;j<g.height;j++)
      for(int i=0;i<g.width;i++) {
        double xc = (2*i+1)/2.0 - xOrg, yc = (2*j+1)/2.0 - yOrg, zc = (2*k+1)/2.0;
        order.push_back(Item{ i,j,k, xc*xc + yc*yc + zc*zc });
      }
  std::stable_sort(order.begin(),order.end(),[](const Item &a,const Item &b){ return a.d > b.d; });

  // Hidden cubes are skipped. In zBuffer mode the pit walls do not hide anything.
  auto value = [&](int x,int y,int z) { return zBuffer ? pit.GetValue2(x,y,z) : pit.GetValue(x,y,z); };
  for(const Item &it : order) {
    if( !pit.GetValue(it.x,it.y,it.z) ) continue;
    bool visible = it.z==0 || !value(it.x+1,it.y,it.z) || !value(it.x-1,it.y,it.z) ||
                   !value(it.x,it.y+1,it.z) || !value(it.x,it.y-1,it.z) || !value(it.x,it.y,it.z-1);
    if( visible ) PitCube(g,it.x,it.y,it.z,out);
  }

}

//-----------------------------------------------------------------------------
// Polycube

static bool HasCube(const PolyCube &p,int x,int y,int z) {
  const BLOCKITEM *c = p.GetCubes();
  for(int i=0;i<p.GetNbCube();i++)
    if( c[i].x==x && c[i].y==y && c[i].z==z ) return true;
  return false;
}

// BlockOut II PolyCube::IsEdgeVisible: an edge is drawn unless it lies inside
// a flat face of the polycube or inside the block
static bool IsEdgeVisible(const PolyCube &p,const BLOCKITEM &c,int edge) {

  int x = c.x, y = c.y, z = c.z;
  bool e1=false,e2=false,e3=false;
  auto f = [&](int a,int b,int d) { return HasCube(p,a,b,d); };
  switch(edge) {
    case 0:  e1=f(x,y,z-1); e2=f(x,y+1,z-1); e3=f(x,y+1,z); break;
    case 1:  e1=f(x,y,z-1); e2=f(x+1,y,z-1); e3=f(x+1,y,z); break;
    case 2:  e1=f(x,y,z-1); e2=f(x,y-1,z-1); e3=f(x,y-1,z); break;
    case 3:  e1=f(x,y,z-1); e2=f(x-1,y,z-1); e3=f(x-1,y,z); break;
    case 4:  e1=f(x,y+1,z); e2=f(x-1,y+1,z); e3=f(x-1,y,z); break;
    case 5:  e1=f(x,y+1,z); e2=f(x+1,y+1,z); e3=f(x+1,y,z); break;
    case 6:  e1=f(x+1,y,z); e2=f(x+1,y-1,z); e3=f(x,y-1,z); break;
    case 7:  e1=f(x,y-1,z); e2=f(x-1,y-1,z); e3=f(x-1,y,z); break;
    case 8:  e1=f(x,y+1,z); e2=f(x,y+1,z+1); e3=f(x,y,z+1); break;
    case 9:  e1=f(x+1,y,z); e2=f(x+1,y,z+1); e3=f(x,y,z+1); break;
    case 10: e1=f(x,y-1,z); e2=f(x,y-1,z+1); e3=f(x,y,z+1); break;
    case 11: e1=f(x-1,y,z); e2=f(x-1,y,z+1); e3=f(x,y,z+1); break;
  }
  return !( (!e1 && !e2 && e3) || (e1 && !e2 && !e3) || (e1 && e2 && e3) );

}

struct Edge { BLOCKITEM a, b; int dir; };   // dir: 1=x 2=y 3=z

static std::vector<Edge> BlockEdges(const PolyCube &p) {

  static const int dir[12] = { 1,2,1,2, 3,3,3,3, 1,2,1,2 };
  std::vector<Edge> edges;
  const BLOCKITEM *cubes = p.GetCubes();
  for(int i=0;i<p.GetNbCube();i++) {
    for(int j=0;j<12;j++) {
      if( !IsEdgeVisible(p,cubes[i],j) ) continue;
      const BLOCKITEM &c = cubes[i];
      Edge e{ { c.x+cubeEdge[j][0][0], c.y+cubeEdge[j][0][1], c.z+cubeEdge[j][0][2] },
              { c.x+cubeEdge[j][1][0], c.y+cubeEdge[j][1][1], c.z+cubeEdge[j][1][2] }, dir[j] };
      auto same = [](const BLOCKITEM &u,const BLOCKITEM &w) { return u.x==w.x && u.y==w.y && u.z==w.z; };
      bool exists = std::any_of(edges.begin(),edges.end(),[&](const Edge &o) {
        return (same(o.a,e.a) && same(o.b,e.b)) || (same(o.a,e.b) && same(o.b,e.a)); });
      if( !exists ) edges.push_back(e);
    }
  }
  return edges;

}

static VERTEX ToSpace(const PitGeometry &g,int x,int y,int z) {
  return v( x*g.cubeSide + g.origin.x, y*g.cubeSide + g.origin.y, z*g.cubeSide + g.origin.z );
}

void BuildBlockLines(const PolyCube &p,const PitGeometry &g,MaterialId mat,std::vector<Vertex> &out) {

  for(const Edge &e : BlockEdges(p))
    Line(out,ToSpace(g,e.a.x,e.a.y,e.a.z),ToSpace(g,e.b.x,e.b.y,e.b.z),mat);

}

void BuildBlockGhost(const PolyCube &p,const PitGeometry &g,std::vector<Vertex> &out) {

  // Faces not touching another cube of the block (BlockOut II PolyCube::CreateGhost)
  static const struct { int nb[3]; int q[4][3]; float n[3]; } face[6] = {
    { { 0, 0,-1}, {{0,0,0},{0,1,0},{1,1,0},{1,0,0}}, { 0, 0,-1} },
    { {-1, 0, 0}, {{0,0,1},{0,1,1},{0,1,0},{0,0,0}}, {-1, 0, 0} },
    { { 0, 0, 1}, {{1,0,1},{1,1,1},{0,1,1},{0,0,1}}, { 0, 0, 1} },
    { { 1, 0, 0}, {{1,0,0},{1,1,0},{1,1,1},{1,0,1}}, { 1, 0, 0} },
    { { 0, 1, 0}, {{0,1,0},{0,1,1},{1,1,1},{1,1,0}}, { 0, 1, 0} },
    { { 0,-1, 0}, {{1,0,0},{1,0,1},{0,0,1},{0,0,0}}, { 0,-1, 0} } };

  const BLOCKITEM *cubes = p.GetCubes();
  for(int i=0;i<p.GetNbCube();i++) {
    const BLOCKITEM &c = cubes[i];
    for(const auto &f : face) {
      if( HasCube(p,c.x+f.nb[0],c.y+f.nb[1],c.z+f.nb[2]) ) continue;
      VERTEX q[4];
      for(int k=0;k<4;k++) q[k] = ToSpace(g,c.x+f.q[k][0],c.y+f.q[k][1],c.z+f.q[k][2]);
      Quad(out,q,v(f.n[0],f.n[1],f.n[2]),MAT_GHOST,VK_LIT);
    }
  }

}

void BuildBlockBigEdges(const PolyCube &p,const PitGeometry &g,float radius,MaterialId mat,std::vector<Vertex> &out) {

  std::vector<Edge> edges = BlockEdges(p);
  const float w = radius * g.cubeSide;
  const int nbDiv = 6;

  // Edge cylinders
  for(const Edge &e : edges) {
    VERTEX a = ToSpace(g,e.a.x,e.a.y,e.a.z);
    VERTEX b = ToSpace(g,e.b.x,e.b.y,e.b.z);
    for(int j=0;j<nbDiv;j++) {
      VERTEX n[4];
      for(int k=0;k<2;k++) {
        float angle = (float)(j+k)/(float)nbDiv * 2.0f * PI;
        float cs = cosf(angle), sn = sinf(angle);
        VERTEX d = e.dir==1 ? v(0,cs,sn) : (e.dir==2 ? v(cs,0,sn) : v(cs,sn,0));
        n[k==0 ? 0 : 2] = d;
      }
      n[1] = n[0]; n[3] = n[2];
      const VERTEX q[4] = { Add(a,w*n[0].x,w*n[0].y,w*n[0].z), Add(b,w*n[1].x,w*n[1].y,w*n[1].z),
                            Add(b,w*n[2].x,w*n[2].y,w*n[2].z), Add(a,w*n[3].x,w*n[3].y,w*n[3].z) };
      Quad(out,q,n,mat,VK_LIT);
    }
  }

  // Corners where 3 edges meet: a small triangle, both windings
  for(int i=0;i<=p.GetWidth();i++)
    for(int j=0;j<=p.GetHeight();j++)
      for(int k=0;k<=p.GetDepth();k++) {
        int dx=0,dy=0,dz=0,count=0;
        for(const Edge &e : edges) {
          const BLOCKITEM *at = nullptr, *other = nullptr;
          if( e.a.x==i && e.a.y==j && e.a.z==k ) { at=&e.a; other=&e.b; }
          if( e.b.x==i && e.b.y==j && e.b.z==k ) { at=&e.b; other=&e.a; }
          if( !at ) continue;
          count++;
          dx += at->x-other->x; dy += at->y-other->y; dz += at->z-other->z;
        }
        // One edge along each axis (BlockOut II GetCornerAt)
        if( count!=3 || !dx || !dy || !dz ) continue;
        VERTEX c = ToSpace(g,i,j,k);
        VERTEX o = v((float)dx,(float)dy,(float)dz);
        VERTEX pz = Add(c,0,0,w*o.z), px = Add(c,w*o.x,0,0), py = Add(c,0,w*o.y,0);
        Tri(out,pz,v(0,0,o.z),Info(mat,VK_LIT)); Tri(out,px,v(o.x,0,0),Info(mat,VK_LIT)); Tri(out,py,v(0,o.y,0),Info(mat,VK_LIT));
        Tri(out,pz,v(0,0,o.z),Info(mat,VK_LIT)); Tri(out,py,v(0,o.y,0),Info(mat,VK_LIT)); Tri(out,px,v(o.x,0,0),Info(mat,VK_LIT));
      }

}

//-----------------------------------------------------------------------------
// Matrices

Matrix Mul(const Matrix &a,const Matrix &b) {
  Matrix r;
  r.Multiply(&b,&a);   // Matrix::Multiply(x,y) computes y*x
  return r;
}

Matrix Translation(float x,float y,float z) {
  Matrix m;
  m.Translate(x,y,z);
  return m;
}

Matrix LookAt(VERTEX eye,VERTEX target,VERTEX up) {

  // gluLookAt
  VERTEX f = v(target.x-eye.x,target.y-eye.y,target.z-eye.z);
  Normalize(&f);
  VERTEX s = v(f.y*up.z-f.z*up.y, f.z*up.x-f.x*up.z, f.x*up.y-f.y*up.x);
  Normalize(&s);
  VERTEX u = v(s.y*f.z-s.z*f.y, s.z*f.x-s.x*f.z, s.x*f.y-s.y*f.x);
  Matrix m;
  m.Init33( s.x, s.y, s.z,  u.x, u.y, u.z,  -f.x,-f.y,-f.z );
  return Mul(m,Translation(-eye.x,-eye.y,-eye.z));

}

Matrix Perspective(float fovyDeg,float aspect,float zNear,float zFar) {

  // gluPerspective (GL clip space, z in -w..w)
  float f = 1.0f / tanf(fovyDeg * PI / 360.0f);
  Matrix m;
  m.Init33( f/aspect,0,0,  0,f,0,  0,0,(zFar+zNear)/(zNear-zFar) );
  m._34 = 2.0f*zFar*zNear/(zNear-zFar);
  m._43 = -1.0f;
  m._44 = 0.0f;
  return m;

}

Matrix GameView() {
  return LookAt(v(0,0,0),v(0,0,10),v(0,1,0));
}

Matrix BlockModelView(const GameLogic &game,const PitGeometry &g,const Matrix &view) {

  const float c = g.cubeSide;
  BLOCKITEM ic = game.GetPolyCube(game.GetPolyCubeIdx()).GetICenter();
  VERTEX center = ToSpace(g,ic.x,ic.y,ic.z);
  VERTEX pos = game.GetBlockPos();
  Matrix m = Mul(view,Translation(pos.x*c,pos.y*c,pos.z*c));
  m = Mul(m,Translation(center.x,center.y,center.z));
  m = Mul(m,game.GetBlockTmpRotation());
  m = Mul(m,game.GetBlockRotation());
  return Mul(m,Translation(-center.x,-center.y,-center.z));

}

Matrix HelpModelView(const GameLogic &game,const PitGeometry &g,const Matrix &view) {

  const float c = g.cubeSide;
  BLOCKITEM ic = game.GetPolyCube(game.GetPolyCubeIdx()).GetICenter();
  VERTEX center = ToSpace(g,ic.x,ic.y,ic.z);
  BLOCKITEM t = game.GetHelpPos();
  Matrix m = Mul(view,Translation(t.x*c,t.y*c,t.z*c));
  m = Mul(m,Translation(center.x,center.y,center.z));
  m = Mul(m,game.GetHelpRotation());
  return Mul(m,Translation(-center.x,-center.y,-center.z));

}

} // namespace bo3
