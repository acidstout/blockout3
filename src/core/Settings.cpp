/*
  File:        Settings.cpp
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
#include "Settings.h"
#include <cctype>
#include <cstring>

namespace bo3 {

static int Clamp(int v,int lo,int hi) { return v<lo ? lo : (v>hi ? hi : v); }

//-----------------------------------------------------------------------------

FxSettings FxFromPreset(int preset) {

  FxSettings f;
  switch(preset) {
    case FX_CLEAN:
      f.scanlines = f.vignette = true;
      break;
    case FX_CRT:
      f.scanlines = f.vignette = f.glow = f.afterglow = f.curvature = f.mask = true;
      break;
    case FX_ARCADE:
      f.scanlines = f.vignette = f.glow = f.afterglow = f.curvature = true;
      f.ega = f.lowRes = true;
      break;
  }
  return f;

}

//-----------------------------------------------------------------------------

void Settings::Sanitize() {

  pitWidth = Clamp(pitWidth,MIN_PITWIDTH,MAX_PITWIDTH);
  pitHeight = Clamp(pitHeight,MIN_PITHEIGHT,MAX_PITHEIGHT);
  pitDepth = Clamp(pitDepth,MIN_PITDEPTH,MAX_PITDEPTH);
  blockSet = Clamp(blockSet,BLOCKSET_FLAT,BLOCKSET_EXTENDED);
  animationSpeed = Clamp(animationSpeed,0,10);
  startLevel = Clamp(startLevel,0,9);
  transparentFace = Clamp(transparentFace,0,10);
  lineWidth = Clamp(lineWidth,0,10);
  fxPreset = Clamp(fxPreset,FX_OFF,FX_CUSTOM);
  soundType = Clamp(soundType,SOUND_BLOCKOUT2,SOUND_BLOCKOUT);

  // Rotation keys are upper case letters/digits (a key array index)
  int *keys[] = { &keyRx1, &keyRy1, &keyRz1, &keyRx2, &keyRy2, &keyRz2 };
  static const char defaults[] = "QWEASD";
  for(int i=0;i<6;i++) {
    int k = toupper(*keys[i] & 0xFF);
    if( !isalnum(k) ) k = defaults[i];
    *keys[i] = k;
  }

}

//-----------------------------------------------------------------------------

GameConfig Settings::MakeGameConfig(int highScore) const {

  GameConfig c;
  c.pitWidth = pitWidth;
  c.pitHeight = pitHeight;
  c.pitDepth = pitDepth;
  c.blockSet = blockSet;
  c.startLevel = startLevel;
  c.animationTime = AnimationTimeFromSpeed(animationSpeed);
  c.soundType = soundType;
  c.setupId = GetSetupId();
  c.highScore = highScore;
  c.keyRx1 = keyRx1; c.keyRy1 = keyRy1; c.keyRz1 = keyRz1;
  c.keyRx2 = keyRx2; c.keyRy2 = keyRy2; c.keyRz2 = keyRz2;
  return c;

}

//-----------------------------------------------------------------------------

bool ImportBlockOut2Setup(const std::vector<byte> &data,Settings &s) {

  // Field sequence of BlockOut II SetupManager::WriteSetup() (SETUP_VERSION 6).
  // Every field is 4 bytes, except the keys (1 byte) and the two 256 byte
  // HTTP strings (skipped).
  size_t pos = 0;
  auto get = [&](int &v) {
    if( pos + 4 > data.size() ) return false;
    int32 x; memcpy(&x,data.data() + pos,4); pos += 4; v = x; return true;
  };
  auto getKey = [&](int &v) {
    if( pos + 1 > data.size() ) return false;
    v = data[pos++]; return true;
  };

  int version;
  if( !get(version) || version<1 || version>6 ) return false;

  Settings r = s;
  int sound = 1, full = 0, windowSize, style, frLimit;
  if( !get(r.pitWidth) || !get(r.pitHeight) || !get(r.pitDepth) || !get(r.blockSet) ||
      !get(r.animationSpeed) ) return false;
  if( version==1 ) {
    // Version 1: 0=slow, 1=medium, 2=fast
    if( r.animationSpeed==1 ) r.animationSpeed = 5;
    if( r.animationSpeed==2 ) r.animationSpeed = 10;
  }
  if( !get(r.startLevel) || !get(sound) || !get(full) || !get(windowSize) ||
      !getKey(r.keyRx1) || !getKey(r.keyRy1) || !getKey(r.keyRz1) ||
      !getKey(r.keyRx2) || !getKey(r.keyRy2) || !getKey(r.keyRz2) ||
      !get(r.transparentFace) ) return false;
  if( version<3 && r.transparentFace ) r.transparentFace = 5;
  r.playSound = sound!=0;
  r.fullScreen = full!=0;

  // Later versions add fields; a short file keeps what was read so far
  if( version>=3 ) {
    pos += 256 + 256 + 3*4;   // HTTP home, proxy, port, use proxy, timeout
    if( version>=4 && get(style) ) get(r.soundType);
    if( version>=5 ) get(frLimit);
    if( version>=6 ) get(r.lineWidth);
  }

  r.Sanitize();
  s = r;
  return true;

}

} // namespace bo3
