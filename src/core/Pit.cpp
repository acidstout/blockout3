/*
  File:        Pit.cpp
  Description: Pit content (logic only)
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
#include "Pit.h"
#include <algorithm>

namespace bo3 {

//-----------------------------------------------------------------------------

void Pit::SetDimension(int gWidth,int gHeight,int gDepth) {

  width  = gWidth;
  height = gHeight;
  depth  = gDepth;
  matrix.assign( (size_t)width * height * depth, 0 );
  Clear();

}

//-----------------------------------------------------------------------------

void Pit::Clear() {

  std::fill(matrix.begin(), matrix.end(), 0);
  tcubeIdx = 1;

}

//-----------------------------------------------------------------------------

bool Pit::IsEmpty() const {

  for(int i=0;i<depth;i++)
    if( !IsLineEmpty(i) ) return false;
  return true;

}

//-----------------------------------------------------------------------------

int Pit::RemoveLines() {

  int nbRemoved = 0;
  int k=depth-1;

  while(k>=0) {
    if( IsLineFull(k) ) {
      RemoveLine(k);
      nbRemoved++;
    } else {
      k--;
    }
  }

  return nbRemoved;

}

//-----------------------------------------------------------------------------

void Pit::RemoveLine(int idx) {

  // Shift every layer above idx one step down (z grows towards the bottom)
  const size_t area = (size_t)width * height;
  int *m = matrix.data();
  for(int k=idx;k>0;k--)
    std::copy(m + (k-1)*area, m + k*area, m + k*area);

  // Clear the top layer
  std::fill(m, m + area, 0);

}

//-----------------------------------------------------------------------------

void Pit::GetOutOfBounds(int x,int y,int z,int *ox,int *oy,int *oz) const {

  *ox = 0;
  *oy = 0;
  *oz = 0;

  if( x<0 )       *ox = -x;
  if( x>=width )  *ox = width - x - 1;
  if( y<0 )       *oy = -y;
  if( y>=height ) *oy = height - y - 1;
  if( z<0 )       *oz = -z;
  if( z>=depth )  *oz = depth - z - 1;

}

//-----------------------------------------------------------------------------

int Pit::GetValue(int x,int y,int z) const {

  if( x<0 || x>=width || y<0 || y>=height || z<0 || z>=depth )
    return 1;
  return matrix[x + y*width + z*width*height];

}

//-----------------------------------------------------------------------------

int Pit::GetValue2(int x,int y,int z) const {

  if( x<0 || x>=width || y<0 || y>=height || z<0 || z>=depth )
    return 0;
  return matrix[x + y*width + z*width*height];

}

//-----------------------------------------------------------------------------

bool Pit::IsLineFull(int z) const {

  for(int j=0;j<height;j++)
    for(int i=0;i<width;i++)
      if( GetValue(i,j,z)<1 ) return false;
  return true;

}

//-----------------------------------------------------------------------------

bool Pit::IsLineEmpty(int z) const {

  for(int j=0;j<height;j++)
    for(int i=0;i<width;i++)
      if( GetValue(i,j,z)!=0 ) return false;
  return true;

}

//-----------------------------------------------------------------------------

void Pit::SetValue(int x,int y,int z,int value) {

  if( x>=0 && x<width && y>=0 && y<height && z>=0 && z<depth )
    matrix[x + y*width + z*width*height] = value;

}

//-----------------------------------------------------------------------------

void Pit::AddCube(int x,int y,int z) {

  tcubeIdx++;
  if(tcubeIdx>NBTCUBE) tcubeIdx=1;
  SetValue(x,y,z,tcubeIdx);

}

} // namespace bo3
