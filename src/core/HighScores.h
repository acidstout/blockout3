/*
  File:        HighScores.h
  Description: High score table (BlockOut II hscore.dat compatible)
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS (SetupManager.cpp)

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

static_assert(sizeof(SCOREREC) == 60, "hscore.dat record layout");

// Identifier of a game setup (585 ids: width and height are interchangeable)
int SetupId(int width,int height,int depth,int blockSet);

class HighScores {

  public:

    // Insert a finished game. Returns its rank (0..9) for the setup, or 10
    // when it did not make the top 10.
    int Insert(const SCOREREC &score);

    // Name of the rank-th score (0..9) of a setup
    void SetName(int setupId,int rank,const char *name);

    // Best score of a setup (0 when none)
    int Best(int setupId) const;

    // Top 10 of a setup, best first (missing entries have empty name "..........")
    void Top10(int setupId,SCOREREC out[10]) const;

    // hscore.dat content: int32 count, then 64-byte records.
    // Parse keeps every complete record of a truncated file; returns false
    // when the content is not a high score file (the table is then empty).
    bool Parse(const std::vector<byte> &data);
    std::vector<byte> Serialize() const;

    const std::vector<SCOREREC> &GetAll() const { return list; }

  private:

    // All scores of all setups, sorted by decreasing score
    std::vector<SCOREREC> list;

};

} // namespace bo3
