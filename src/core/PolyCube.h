/*
  File:        PolyCube.h
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
#pragma once

#include "CoreTypes.h"

namespace bo3 {

class PolyCube
{
  public:

    // Add a cube to the polycube
    void AddCube(int x,int y,int z);

    // Set polycube info
    void SetInfo(int highScore,int lowScore,bool flat,bool basic);

    // Initialise the rotation center (call once all cubes are added)
    void InitRotationCenter();

    // Return the rotation center (cube coordinates)
    BLOCKITEM GetICenter() const { return iCenter; }

    // Copy the polycube cube coordinates
    void CopyCube(BLOCKITEM *c,int *nb) const;
    const BLOCKITEM *GetCubes() const { return cubes; }

    int  GetNbCube() const { return nbCube; }
    int  GetHighScore() const { return hScore; }
    int  GetLowScore() const { return lScore; }

    // Check if the polycube belongs to the set
    bool IsInSet(int set) const;

    // Get dimension
    int GetWidth() const;
    int GetHeight() const;
    int GetDepth() const;
    int GetMaxDim() const;

    // Orientation stuff (Used by AI player)
    void AddOrientation(int r0,int r1,int r2);
    const ORIENTATION *GetOrientationAt(int idx) const { return &allRot[idx]; }
    int GetNbOrientation() const { return nbOrientation; }

  private:

    int  hScore = 0;      // score (dropped from top position)
    int  lScore = 0;      // score (non dropped)
    bool isFlat = false;  // Into the flat set
    bool isBasic = false; // Into the basic set
    BLOCKITEM iCenter{};  // Rotation center (cube coordinates)

    BLOCKITEM cubes[MAX_CUBE]{};
    int nbCube = 0;

    ORIENTATION allRot[24]{};
    int nbOrientation = 0;

};

// Initialise the 41 polycubes of the game
void InitAllPolyCubes(PolyCube *allPolyCube);

} // namespace bo3
