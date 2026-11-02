/*
  File:        Settings.h
  Description: User settings
  Program:     Block Out III

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

#include "GameLogic.h"
#include "HighScores.h"
#include <vector>

namespace bo3 {

// Retro screen effects
enum FxPreset { FX_OFF, FX_CLEAN, FX_CRT, FX_ARCADE, FX_CUSTOM };

struct FxSettings {
  bool scanlines = false;
  bool vignette = false;
  bool glow = false;
  bool afterglow = false;
  bool curvature = false;
  bool mask = false;       // aperture grille, only applied from 1440p up
  bool ega = false;        // 16 color EGA palette
  bool lowRes = false;     // 320x200 rendering
};

FxSettings FxFromPreset(int preset);

struct Settings {
  // Game setup
  int  pitWidth = 5;
  int  pitHeight = 5;
  int  pitDepth = 12;
  int  blockSet = BLOCKSET_FLAT;
  int  animationSpeed = 5;        // 0..10
  int  startLevel = 0;            // 0..9
  // Rotation keys
  int  keyRx1 = 'Q', keyRy1 = 'W', keyRz1 = 'E';
  int  keyRx2 = 'A', keyRy2 = 'S', keyRz2 = 'D';
  // Graphics
  int  transparentFace = 0;       // 0..10
  int  lineWidth = 0;             // 0..10
  bool fullScreen = false;
  int  fxPreset = FX_CLEAN;
  FxSettings fx = FxFromPreset(FX_CLEAN);
  // Sound
  bool playSound = true;
  int  soundType = SOUND_BLOCKOUT2;

  // Clamp every value into its valid range
  void Sanitize();

  int  GetSetupId() const { return SetupId(pitWidth,pitHeight,pitDepth,blockSet); }
  GameConfig MakeGameConfig(int highScore) const;
};

// Read the game related part of a BlockOut II setup.dat (versions 1..6).
// Returns false when the data is not a setup file.
bool ImportBlockOut2Setup(const std::vector<byte> &data,Settings &s);

} // namespace bo3
