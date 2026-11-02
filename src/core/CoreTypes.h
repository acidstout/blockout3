/*
  File:        CoreTypes.h
  Description: Basic types and constants of the game logic (no graphics, no OS)
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

#include <cstdint>

namespace bo3 {

using int32  = std::int32_t;
using uint32 = std::uint32_t;
using byte   = std::uint8_t;

constexpr float PI          = 3.1415926535f;
constexpr int   MAX_CUBE    = 50;
constexpr int   NB_POLYCUBE = 41;
constexpr int   MAX_MOVE    = 256;
constexpr int   NBTCUBE     = 16;

// Pit dimension limits
constexpr int MAX_PITWIDTH  = 7;
constexpr int MAX_PITHEIGHT = 7;
constexpr int MAX_PITDEPTH  = 18;
constexpr int MIN_PITWIDTH  = 3;
constexpr int MIN_PITHEIGHT = 3;
constexpr int MIN_PITDEPTH  = 6;

// Block sets
constexpr int BLOCKSET_FLAT     = 0;
constexpr int BLOCKSET_BASIC    = 1;
constexpr int BLOCKSET_EXTENDED = 2;

// Game state
constexpr int GAME_PLAYING = 1;
constexpr int GAME_PAUSED  = 2;
constexpr int GAME_OVER    = 3;
constexpr int GAME_DEMO    = 4;

// Sound presets
constexpr int SOUND_BLOCKOUT2 = 0;
constexpr int SOUND_BLOCKOUT  = 1;

// Key codes of the key state array. The values are those of SDL 1.2 so that
// key bindings stored in setup.dat by BlockOut II stay valid. Printable keys
// use their ASCII code.
enum Key : int {
  KEY_BACKSPACE = 8, KEY_RETURN = 13, KEY_ESCAPE = 27, KEY_SPACE = 32, KEY_DELETE = 127,
  KEY_UP = 273, KEY_DOWN = 274, KEY_RIGHT = 275, KEY_LEFT = 276,
  KEY_INSERT = 277, KEY_HOME = 278, KEY_END = 279,
  KEY_PAGEUP = 280, KEY_PAGEDOWN = 281, KEY_F1 = 282,
  KEY_COUNT = 323
};

struct VERTEX {
  float x;
  float y;
  float z;
};

struct BLOCKITEM {
  int x;
  int y;
  int z;
};

struct ORIENTATION {
  int r0;
  int r1;
  int r2;
};

struct AI_MOVE {
  int32 rotate;
  int32 tx;
  int32 ty;
  int32 tz;
};

// Score of a game. Field order and types match the 60 first bytes of the
// BlockOut II hscore.dat record.
struct SCOREREC {
  int32  setupId;
  int32  score;
  int32  nbCube;
  int32  nbLine1;
  int32  nbLine2;
  int32  nbLine3;
  int32  nbLine4;
  int32  nbLine5;
  int32  startLevel;
  uint32 date;
  char   name[11];
  byte   emptyPit;
  int32  scoreId;
  float  gameTime;
};

inline VERTEX v(float x, float y, float z) { return VERTEX{x, y, z}; }

// Round to the nearest integer (half away from zero), as BlockOut II does
inline int fround(float x) {
  if (x < 0.0f) return -(int)((-x) + 0.5f);
  return (int)(x + 0.5f);
}

void Normalize(VERTEX *v);

// Pseudo random generator. Same sequence as the MSVC CRT rand()/srand(), so
// games are reproducible on every build and match BlockOut II.
class Random {
public:
  explicit Random(uint32 seed = 1) : state(seed) {}
  void Seed(uint32 seed) { state = seed; }
  int Next() {
    state = state * 214013u + 2531011u;
    return (int)((state >> 16) & 0x7FFF);
  }
private:
  uint32 state;
};

} // namespace bo3
