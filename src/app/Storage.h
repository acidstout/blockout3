/*
  File:        Storage.h
  Description: Settings and high score files in %LOCALAPPDATA%\Block Out III
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

#include <windows.h>
#include "HighScores.h"
#include "Settings.h"

namespace bo3 {

// Loads settings.ini; on the very first start imports BlockOut II setup.dat.
// placement receives the saved window position (length 0 when none).
void LoadSettings(Settings &s,WINDOWPLACEMENT &placement);
void SaveSettings(const Settings &s,const WINDOWPLACEMENT &placement);

// Loads hscore.dat; on the very first start imports the BlockOut II one.
void LoadHighScores(HighScores &hs);
// Returns false (and keeps the previous file) when the file cannot be written
bool SaveHighScores(const HighScores &hs);

} // namespace bo3
