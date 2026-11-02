/*
  File:        Scene.h
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
#pragma once

#include <cstdint>
#include <vector>
#include "GameLogic.h"

namespace bo3 {

// Materials (BlockOut II GLMATERIAL values), index stored in each vertex
enum MaterialId : uint32_t {
  MAT_BLACK, MAT_GRID, MAT_BACK, MAT_WHITE, MAT_RED, MAT_GRAY, MAT_GHOST,
  MAT_MENU_GRID, MAT_MENU_RED, MAT_MENU_BLUE,
  MAT_LEVEL0,   // 7 pit layer colours follow
  MAT_COUNT = MAT_LEVEL0 + 7
};

struct Material {
  float ambient[4];    // also the colour of unlit primitives
  float diffuse[4];
  float specular[4];   // rgb + shininess
};
void GetMaterials(Material out[MAT_COUNT],int transparency);

// How a vertex is shaded
enum VertexKind : uint32_t { VK_UNLIT, VK_LIT, VK_LINE };

// Triangle list vertex. Lines are quads of 6 vertices expanded on screen by
// the vertex shader: pos = this end, aux = the other end.
// For lit triangles aux is the normal.
struct Vertex {
  float    pos[3];
  float    aux[3];
  uint32_t info;     // material | kind<<8 | lineEnd<<12 | lineSide<<13
};

// Pit placement in 3D space (BlockOut II Pit::Create)
struct PitGeometry {
  float  cubeSide;
  VERTEX origin;
  float  fWidth, fHeight, fDepth;
  int    width, height, depth;
};
PitGeometry MakePitGeometry(int width,int height,int depth);

// The pit: walls, grid and the stacked cubes, in drawing order (painter's
// algorithm, BlockOut II Pit::Render). zBuffer: game over animation mode.
void BuildPit(const Pit &pit,const PitGeometry &g,bool renderCubes,bool zBuffer,std::vector<Vertex> &out);

// A polycube in its own space: outline lines, ghost faces, big round edges
void BuildBlockLines(const PolyCube &p,const PitGeometry &g,MaterialId mat,std::vector<Vertex> &out);
void BuildBlockGhost(const PolyCube &p,const PitGeometry &g,std::vector<Vertex> &out);
void BuildBlockBigEdges(const PolyCube &p,const PitGeometry &g,float radius,MaterialId mat,std::vector<Vertex> &out);

// GL style matrices (column vectors)
Matrix LookAt(VERTEX eye,VERTEX target,VERTEX up);
Matrix Perspective(float fovyDeg,float aspect,float zNear,float zFar);
Matrix Translation(float x,float y,float z);
Matrix Mul(const Matrix &a,const Matrix &b);   // a * b

// Camera of the game: at the origin looking down the pit (+z)
Matrix GameView();

// Model-view matrix of the falling block and of the practice help block
Matrix BlockModelView(const GameLogic &game,const PitGeometry &g,const Matrix &view);
Matrix HelpModelView(const GameLogic &game,const PitGeometry &g,const Matrix &view);

} // namespace bo3
