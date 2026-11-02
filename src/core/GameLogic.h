/*
  File:        GameLogic.h
  Description: Game rules and state machine (no graphics, no OS)
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS (Game.cpp)

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

#include "BotPlayer.h"
#include "Matrix.h"
#include "Pit.h"
#include "PolyCube.h"

namespace bo3 {

enum class Sound { Blub, Wozz, Tchh, Line, Level, Empty, WellDone,
                   Line2, Level2, Empty2, WellDone2, Hit };

class SoundSink {
public:
  virtual ~SoundSink() = default;
  virtual void Play(Sound s) = 0;
};

// Everything a game needs from the setup, captured when the game starts
struct GameConfig {
  int   pitWidth = 5;
  int   pitHeight = 5;
  int   pitDepth = 12;
  int   blockSet = BLOCKSET_FLAT;
  int   startLevel = 0;
  float animationTime = 0.1f;
  int   soundType = SOUND_BLOCKOUT2;
  int   setupId = 0;
  int   highScore = 0;
  // Rotation keys (upper case letters or key codes)
  int   keyRx1 = 'Q', keyRy1 = 'W', keyRz1 = 'E';
  int   keyRx2 = 'A', keyRy2 = 'S', keyRz2 = 'D';
};

// Block animation time (seconds) for an animation speed setting (0..10)
inline float AnimationTimeFromSpeed(int speed) {
  const float min = 0.05f;
  const float max = 0.15f;
  return min + (max-min) * (float)(10 - speed) / 10.0f;
}

// Camera of the "game over" pit animation
struct EndCamera {
  VERTEX eye;
  VERTEX target;
  VERTEX up;
};

class GameLogic {

  public:
    GameLogic();

    void SetSoundSink(SoundSink *sink) { soundSink = sink; }

    // Start a game, a demo (bot plays) or a practice (no descent, help key)
    // fTime: current time in seconds, seed: random seed, date: unix time
    void StartGame(const GameConfig &cfg,float fTime,uint32 seed,uint32 date);
    void StartDemo(const GameConfig &cfg,float fTime,uint32 seed,uint32 date);
    void StartPractice(const GameConfig &cfg,float fTime,uint32 seed,uint32 date);

    // Process one frame. keys: key state array (KEY_COUNT entries), consumed
    // keys are cleared. Returns 0 to continue, 1 when a game ends (back to the
    // menu / high score), 2 when a demo or practice ends.
    int Process(byte *keys,float fTime);

    // ---- State for rendering and tests ----------------------------------
    const Pit       &GetPit() const { return thePit; }
    const PolyCube  &GetPolyCube(int idx) const { return allPolyCube[idx]; }
    const SCOREREC  &GetScore() const { return score; }
    SCOREREC        &GetScore() { return score; }
    int   GetMode() const { return gameMode; }
    int   GetLevel() const { return level; }
    int   GetHighScore() const { return highScore; }
    int   GetPolyCubeIdx() const { return pIdx; }
    bool  IsDemo() const { return demoFlag; }
    bool  IsPractice() const { return practiceFlag; }
    bool  IsRedMode() const { return redMode; }
    int   GetBlockSet() const { return config.blockSet; }
    float GetGameStartTime() const { return startGameTime; }
    float GetCurrentTime() const { return curTime; }

    // Current block: animated position (cube units), rotation in progress
    // (tmpRotation) and current rotation (matRot).
    // World transform = T(pos*cubeSide) * T(center) * tmpRot * rot * T(-center)
    VERTEX        GetBlockPos() const { return vPos; }
    const Matrix &GetBlockRotation() const { return matRot; }
    const Matrix &GetBlockTmpRotation() const { return tmpRotation; }
    BLOCKITEM     GetBlockGridPos() const { return BLOCKITEM{xPos,yPos,zPos}; }

    // Spark (blocked rotation): start time (0 when inactive) and cube
    float     GetSparkTime() const { return startSpark; }
    BLOCKITEM GetSparkPos() const { return sparkPos; }

    // Game over animation
    bool      IsEndAnimStarted() const { return endAnimStarted; }
    EndCamera GetEndCamera() const { return endCamera; }

    // Practice help: suggested final rotation and position
    float         GetHelpTime() const { return startShowAI; }
    const Matrix &GetHelpRotation() const { return helpRot; }
    BLOCKITEM     GetHelpPos() const { return helpPos; }

    // Internal state, for the regression tests
    int   DebugRotateMode() const { return rotateMode; }
    bool  DebugDropMode() const { return dropMode; }
    float DebugEndTime() const { return startEndTime; }

  private:

    void HandleKey(byte *keys);
    bool StartRotate(int rType);
    bool StartTranslate(int tType);
    void InitTranslate();
    void StartDrop();
    void AddPolyCube();
    void NewPolyCube(byte *keys);
    int  SelectPolyCube();
    void TransformCube(const Matrix *matRot,BLOCKITEM *cubes,int nbCube,int tx,int ty,int tz) const;
    bool IsOverlap(const Matrix *matRot,int tx,int ty,int tz,int *ox=nullptr,int *oy=nullptr,int *oz=nullptr,BLOCKITEM *pos=nullptr) const;
    bool IsLower() const;
    int  GetBottom() const;
    void ComputeScore(int nbLines,bool pitEmpty);
    void StartSpark(const BLOCKITEM *pos);
    void ComputeHelp();
    void Play(Sound s) { if(soundSink) soundSink->Play(s); }

    // The pit and the polycubes
    Pit      thePit;
    PolyCube allPolyCube[NB_POLYCUBE];
    int      possible[NB_POLYCUBE] = {};
    int      nbPossible = 0;

    GameConfig config;
    SoundSink *soundSink = nullptr;
    Random     rng;

    // Game stuff
    SCOREREC score{};
    int   level = 0;
    int   highScore = 0;
    int   gameMode = GAME_OVER;
    int   pIdx = 0;
    int   exitValue = 0;
    int   cubePerLevel = 0;
    int   dropPos = 0;
    int   cursorPos = 0;

    // AI stuff (Demo mode)
    BotPlayer botPlayer;
    AI_MOVE AIMoves[MAX_MOVE] = {};
    int     nbAIMove = 0;
    int     curAIMove = 0;
    float   lastAIMoveTime = 0.0f;
    bool    demoFlag = false;
    bool    practiceFlag = false;

    // PolyCube coordinates
    Matrix  matRot;
    Matrix  newMatRot;
    Matrix  tmpRotation;
    VERTEX  vPos{};
    VERTEX  vTransPos{};
    VERTEX  vOrgPos{};
    int     xPos = 0;
    int     yPos = 0;
    int     zPos = 0;

    // Time stamps
    float startTranslateTime = 0.0f;
    float startRotateTime = 0.0f;
    float startRedTime = 0.0f;
    float startPauseTime = 0.0f;
    float startStepTime = 0.0f;
    float startGameTime = 0.0f;
    float startSpark = 0.0f;
    float startEndTime = 0.0f;

    float animationTime = 0.1f;
    float stepTime = 0.0f;

    // Modes
    int   rotateMode = 0;
    bool  dropMode = false;
    bool  dropped = false;
    bool  lastKeyMode = false;
    bool  redMode = false;

    float curTime = 0.0f;
    bool  endAnimStarted = false;
    EndCamera endCamera{};
    BLOCKITEM sparkPos{};

    // Constant matrices
    Matrix matRotOx, matRotOy, matRotOz;
    Matrix matRotNOx, matRotNOy, matRotNOz;

    // Help mode
    Matrix    helpRot;
    BLOCKITEM helpPos{};
    float     startShowAI = 0.0f;

};

} // namespace bo3
