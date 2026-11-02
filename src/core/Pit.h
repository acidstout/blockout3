/*
  File:        Pit.h
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
#pragma once

#include "CoreTypes.h"
#include <vector>

namespace bo3 {

class Pit
{
  public:

    // Initialise pit dimension (clears the pit)
    void SetDimension(int gWidth,int gHeight,int gDepth);

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetDepth() const { return depth; }

    // Clear the pit
    void Clear();

    // Get the value at the specified coordinates
    // Return 1 when (x,y,z) if out of the pit
    int GetValue(int x,int y,int z) const;

    // Get the value at the specified coordinates
    // Return 0 when (x,y,z) if out of the pit
    int GetValue2(int x,int y,int z) const;

    // Add a cube at the specified coordinates
    void AddCube(int x,int y,int z);

    // Get "out of bounds" values
    void GetOutOfBounds(int x,int y,int z,int *ox,int *oy,int *oz) const;

    // Remove full lines, return the number of removed lines
    int RemoveLines();

    // Return true if the pit is empty
    bool IsEmpty() const;

    bool IsLineFull(int z) const;
    bool IsLineEmpty(int z) const;

    // Raw content (width*height*depth values, x fastest)
    const std::vector<int> &GetMatrix() const { return matrix; }

  private:

    void SetValue(int x,int y,int z,int value);
    void RemoveLine(int idx);

    int width = 0;
    int height = 0;
    int depth = 0;
    int tcubeIdx = 1;
    std::vector<int> matrix;

};

} // namespace bo3
