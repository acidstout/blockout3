/*
  File:        HighScores.cpp
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
#include "HighScores.h"
#include <cstring>

namespace bo3 {

static const int RECORD_SIZE = 64;   // 60 bytes of SCOREREC + 4 unused
static const int MAX_RECORDS = 10 * 585;

//-----------------------------------------------------------------------------

int SetupId(int width,int height,int depth,int blockSet) {

  // Same ids as the BlockOut II setupId[] table: 15 distinct {width,height}
  // pairs per depth, 13 depths per block set
  static const int pair[5][5] = { { 0, 1, 2, 4, 6 },
                                  { 1, 3, 5, 7, 9 },
                                  { 2, 5, 8,10,11 },
                                  { 4, 7,10,12,13 },
                                  { 6, 9,11,13,14 } };
  return blockSet*195 + (depth-MIN_PITDEPTH)*15 +
         pair[width-MIN_PITWIDTH][height-MIN_PITHEIGHT];

}

//-----------------------------------------------------------------------------

int HighScores::Insert(const SCOREREC &score) {

  // Null scores are not recorded
  if( score.score == 0 ) return 10;

  // Insertion point: before the first lower score; rank counts the better
  // scores of the same setup
  int rank = 0;
  size_t at = 0;
  while( at<list.size() && rank<10 && score.score <= list[at].score ) {
    if( list[at].setupId == score.setupId ) rank++;
    at++;
  }
  if( rank>=10 ) return 10;

  list.insert(list.begin() + at, score);

  // Keep the 10 best scores of the setup
  int n = 0;
  for(size_t i=0;i<list.size();) {
    if( list[i].setupId == score.setupId && ++n>10 )
      list.erase(list.begin() + i);
    else
      i++;
  }

  return rank;

}

//-----------------------------------------------------------------------------

void HighScores::SetName(int setupId,int rank,const char *name) {

  for(SCOREREC &s : list) {
    if( s.setupId!=setupId ) continue;
    if( rank-- == 0 ) {
      strncpy_s(s.name,sizeof(s.name),name,_TRUNCATE);
      return;
    }
  }

}

//-----------------------------------------------------------------------------

int HighScores::Best(int setupId) const {

  for(const SCOREREC &s : list)
    if( s.setupId == setupId ) return s.score;
  return 0;

}

//-----------------------------------------------------------------------------

void HighScores::Top10(int setupId,SCOREREC out[10]) const {

  int n = 0;
  for(const SCOREREC &s : list)
    if( s.setupId == setupId && n<10 ) out[n++] = s;

  for(;n<10;n++) {
    memset(&out[n],0,sizeof(SCOREREC));
    memcpy(out[n].name,"..........",11);
  }

}

//-----------------------------------------------------------------------------

bool HighScores::Parse(const std::vector<byte> &data) {

  list.clear();
  if( data.size() < 4 ) return false;

  int32 count;
  memcpy(&count,data.data(),4);
  if( count<1 || count>MAX_RECORDS ) return false;

  // A truncated file keeps its complete records
  size_t available = (data.size() - 4) / RECORD_SIZE;
  if( (size_t)count > available ) count = (int32)available;

  for(int i=0;i<count;i++) {
    SCOREREC s;
    memcpy(&s,data.data() + 4 + (size_t)i*RECORD_SIZE,sizeof(SCOREREC));
    s.name[10] = 0;
    if( s.setupId<0 || s.setupId>=585 ) continue;
    list.push_back(s);
  }
  return true;

}

//-----------------------------------------------------------------------------

std::vector<byte> HighScores::Serialize() const {

  std::vector<byte> data(4 + list.size()*RECORD_SIZE, 0);
  int32 count = (int32)list.size();
  memcpy(data.data(),&count,4);
  for(size_t i=0;i<list.size();i++)
    memcpy(data.data() + 4 + i*RECORD_SIZE,&list[i],sizeof(SCOREREC));
  return data;

}

} // namespace bo3
