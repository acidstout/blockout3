/*
  File:        Synth.h
  Description: Procedural sound effects (no OS)
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

#include <cstdint>
#include <vector>
#include "GameLogic.h"

namespace bo3 {

constexpr int SYNTH_RATE = 44100;   // mono, 16 bit

// Renders one game sound. The "2" sounds imitate the PC speaker of the
// 1989 DOS Blockout; the others follow the BlockOut II samples.
std::vector<int16_t> Synthesize(Sound s);

const char *SoundName(Sound s);
constexpr int SOUND_COUNT = (int)Sound::Hit + 1;

} // namespace bo3
