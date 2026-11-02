/*
  File:        PolyCube.cpp
  Description: PolyCube shape and properties (logic only)
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "PolyCube.h"

namespace bo3 {

//-----------------------------------------------------------------------------

void PolyCube::SetInfo(int highScore,int lowScore,bool flat,bool basic) {

  hScore = highScore;
  lScore = lowScore;
  isFlat = flat;
  isBasic = basic;

}

//-----------------------------------------------------------------------------

void PolyCube::AddCube(int x,int y,int z) {

  if( nbCube<MAX_CUBE ) {
    cubes[nbCube].x = x;
    cubes[nbCube].y = y;
    cubes[nbCube].z = z;
    nbCube++;
  }

}

//-----------------------------------------------------------------------------

void PolyCube::AddOrientation(int r0,int r1,int r2) {

  if( nbOrientation<24 ) {
    allRot[nbOrientation].r0 = r0;
    allRot[nbOrientation].r1 = r1;
    allRot[nbOrientation].r2 = r2;
    nbOrientation++;
  }

}

//-----------------------------------------------------------------------------

bool PolyCube::IsInSet(int set) const {

  switch(set) {
    case BLOCKSET_EXTENDED:
      return true;
    case BLOCKSET_BASIC:
      return isBasic;
    case BLOCKSET_FLAT:
      return isFlat;
  }

  return false;

}

//-----------------------------------------------------------------------------

int PolyCube::GetWidth() const {
  int maxW = 0;
  for(int i=0;i<nbCube;i++)
    if(cubes[i].x > maxW) maxW = cubes[i].x;
  return maxW + 1;
}

int PolyCube::GetHeight() const {
  int maxH = 0;
  for(int i=0;i<nbCube;i++)
    if(cubes[i].y > maxH) maxH = cubes[i].y;
  return maxH + 1;
}

int PolyCube::GetDepth() const {
  int maxD = 0;
  for(int i=0;i<nbCube;i++)
    if(cubes[i].z > maxD) maxD = cubes[i].z;
  return maxD + 1;
}

int PolyCube::GetMaxDim() const {
  int maxDim = GetWidth();
  if( GetHeight() > maxDim ) maxDim = GetHeight();
  if( GetDepth() > maxDim ) maxDim = GetDepth();
  return maxDim;
}

//-----------------------------------------------------------------------------

void PolyCube::InitRotationCenter() {

  // Emulate "BlockOut original" rotation center
  iCenter.x = GetWidth() - 1;
  iCenter.y = 1;
  iCenter.z = GetDepth() - 1;

}

//-----------------------------------------------------------------------------

void PolyCube::CopyCube(BLOCKITEM *c,int *nb) const {

  for(int i=0;i<nbCube;i++)
    c[i] = cubes[i];
  *nb = nbCube;

}

} // namespace bo3
