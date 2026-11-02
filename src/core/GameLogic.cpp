/*
  File:        GameLogic.cpp
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
#include "GameLogic.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace bo3 {

// Score calculation factors, used by ComputeScore()
// (coming from measurements made with the original BlockOut game)

// PolyCube Level Factor (from level 0 to 10)
static const float pLevelFactor[] = { 0.066990f , 0.139195f , 0.219800f , 0.308444f , 0.403897f ,
                                      0.507822f , 0.619062f , 0.738630f , 0.865802f , 1.000000f ,
                                      1.133333f };

// Depth Factor (from depth 6 to 18)
static const float depthFactor[] = { 0.0f , 0.0f , 0.0f , 0.0f , 0.0f , 0.0f ,
                                     1.557692f, 1.367521f, 1.217949f, 1.100427f, 1.000000f,
                                     0.918803f, 0.852564f, 0.788996f, 0.737714f, 0.691774f,
                                     0.651709f, 0.614850f, 0.583868f };

// Line Level Factor (from level 0 to 10)
static const float lLeveFactor[] = { 0.096478f, 0.163873f, 0.242913f, 0.328261f, 0.422329f,
                                     0.518394f, 0.630405f, 0.747501f, 0.867087f, 1.000000f,
                                     1.131653f };

// Line number factor (from 1 to 5)
static const float lNumberFactor[] = { 0.0f , 1.000000f, 3.703372f, 8.104827f, 14.188325f, 22.144941f };

// Line base score (FLAT,BASIC,EXTENDED)
static const float lineBase[] = { 762.5f , 875.5f , 2886.25f };

// Step Time base (seconds)
static const float timeBase = 5.51f;

// Time Level factor
static const float timeLevelFactor = 0.64f;

// Drop time (seconds)
static const float dropTime = 0.16f;

// Spark time
static const float sparkTime = 0.5f;

// Pit start depth in 3D space (used by the end camera)
static const float STARTZ = 0.87f;

// ---------------------------------------------------------------------

GameLogic::GameLogic() {

  // Default rotation matrices

  matRotOx.Init33( 1.0f , 0.0f , 0.0f ,
                   0.0f , 0.0f ,-1.0f ,
                   0.0f , 1.0f , 0.0f );

  matRotOy.Init33( 0.0f , 0.0f ,-1.0f ,
                   0.0f , 1.0f , 0.0f ,
                   1.0f , 0.0f , 0.0f );

  matRotOz.Init33( 0.0f ,-1.0f , 0.0f ,
                   1.0f , 0.0f , 0.0f ,
                   0.0f , 0.0f , 1.0f );

  matRotNOx.Init33( 1.0f , 0.0f , 0.0f ,
                    0.0f , 0.0f , 1.0f ,
                    0.0f ,-1.0f , 0.0f );

  matRotNOy.Init33( 0.0f , 0.0f , 1.0f ,
                    0.0f , 1.0f , 0.0f ,
                   -1.0f , 0.0f , 0.0f );

  matRotNOz.Init33( 0.0f , 1.0f , 0.0f ,
                   -1.0f , 0.0f , 0.0f ,
                    0.0f , 0.0f , 1.0f );

  InitAllPolyCubes(allPolyCube);

}

// ---------------------------------------------------------------------

static int Clamp(int v,int lo,int hi) { return v<lo ? lo : (v>hi ? hi : v); }

void GameLogic::StartGame(const GameConfig &c,float fTime,uint32 seed,uint32 date) {

  // Same limits as the BlockOut II setup
  GameConfig cfg = c;
  cfg.pitWidth   = Clamp(cfg.pitWidth,MIN_PITWIDTH,MAX_PITWIDTH);
  cfg.pitHeight  = Clamp(cfg.pitHeight,MIN_PITHEIGHT,MAX_PITHEIGHT);
  cfg.pitDepth   = Clamp(cfg.pitDepth,MIN_PITDEPTH,MAX_PITDEPTH);
  cfg.blockSet   = Clamp(cfg.blockSet,BLOCKSET_FLAT,BLOCKSET_EXTENDED);
  cfg.startLevel = Clamp(cfg.startLevel,0,9);
  config = cfg;

  if( cfg.pitWidth!=thePit.GetWidth() || cfg.pitHeight!=thePit.GetHeight() ||
      cfg.pitDepth!=thePit.GetDepth() )
    thePit.SetDimension(cfg.pitWidth,cfg.pitHeight,cfg.pitDepth);

  // Initialise game variables
  animationTime = cfg.animationTime;
  level = cfg.startLevel;
  pIdx = 0;
  gameMode = GAME_PLAYING;
  thePit.Clear();
  stepTime = timeBase * powf(timeLevelFactor,(float)level);
  exitValue = 0;
  cubePerLevel = thePit.GetHeight() * 15 + thePit.GetWidth() * 15;
  highScore = cfg.highScore;
  startGameTime = fTime;
  curTime = fTime;
  endAnimStarted = false;
  demoFlag = false;
  practiceFlag = false;

  // Initialise score structure
  memset(&score,0,sizeof(SCOREREC));
  score.date = date;
  score.startLevel = level;
  score.setupId = cfg.setupId;

  // Get all possible polycubes
  nbPossible = 0;
  int minDim = thePit.GetWidth();
  if( thePit.GetHeight() < minDim ) minDim = thePit.GetHeight();
  for(int i=0;i<NB_POLYCUBE;i++) {
    if( allPolyCube[i].IsInSet(cfg.blockSet) ) {
      if( allPolyCube[i].GetMaxDim() <= minDim ) {
        possible[nbPossible] = i;
        nbPossible++;
      }
    }
  }

  rng.Seed(seed);

  NewPolyCube(nullptr);

}

// ---------------------------------------------------------------------

void GameLogic::StartDemo(const GameConfig &cfg,float fTime,uint32 seed,uint32 date) {

  StartGame(cfg,fTime,seed,date);
  botPlayer.Init(thePit.GetWidth(),thePit.GetHeight(),thePit.GetDepth(),cfg.blockSet);
  botPlayer.GetMoves(&thePit,&(allPolyCube[pIdx]),xPos,yPos,zPos,AIMoves,&nbAIMove);
  gameMode = GAME_DEMO;
  demoFlag = true;

}

// ---------------------------------------------------------------------

void GameLogic::StartPractice(const GameConfig &cfg,float fTime,uint32 seed,uint32 date) {

  StartGame(cfg,fTime,seed,date);
  practiceFlag = true;
  // Initialise the bot player for the help mode
  startShowAI = 0.0f;
  botPlayer.Init(thePit.GetWidth(),thePit.GetHeight(),thePit.GetDepth(),cfg.blockSet);

}

// ---------------------------------------------------------------------

int GameLogic::Process(byte *keys,float fTime) {

  curTime = fTime;

  if( gameMode == GAME_OVER ) {

    // Process pit animation
    float pTime = fTime - startEndTime;
    if( pTime>0.0f ) {

      endAnimStarted = true;

      float cSide = 1.0f / (float)( thePit.GetWidth()>thePit.GetHeight() ? thePit.GetWidth() : thePit.GetHeight() );
      float zc = 5.0f * cSide;
      float d  = 10.0f * cSide;
      VERTEX eye;
      VERTEX vUp;

      if( pTime<4.0f ) {

        eye = v( 0.0f , -d * (pTime/4.0f) , 0.0f );
        vUp = v( 0.0f , 1.0f , -1.0f * (pTime/4.0f) );
        Normalize(&vUp);

      } else {

        float a = (pTime-4.0f)/2.0f;
        eye = v( d * sinf(a) , -d * cosf(a) , 0.0f );
        vUp = v( -eye.x , -eye.y , -1.0f );
        Normalize(&vUp);

      }

      endCamera.eye = eye;
      endCamera.target = v( 0.0f , 0.0f , zc + STARTZ );
      endCamera.up = vUp;

    }

  }

  switch( gameMode ) {

    case GAME_PLAYING:
    case GAME_PAUSED:
    case GAME_OVER:
      HandleKey(keys);
      break;

    case GAME_DEMO: {

      // Abort the demo
      if( keys[KEY_ESCAPE] ) {
        score.gameTime = curTime - startGameTime;
        gameMode = GAME_OVER;
        keys[KEY_ESCAPE]=0;
        return 2;
      }

      // Wait end of move
      float aTime = curTime - lastAIMoveTime;
      if( aTime>animationTime && rotateMode==0 ) {

        if( curAIMove>=nbAIMove ) {

          StartDrop();

        } else {

          startRotateTime = curTime;
          rotateMode = AIMoves[curAIMove].rotate;
          switch( rotateMode ) {
           case 1: newMatRot.Multiply(&matRot,&matRotOx); break;
           case 2: newMatRot.Multiply(&matRot,&matRotOy); break;
           case 3: newMatRot.Multiply(&matRot,&matRotOz); break;
           case 4: newMatRot.Multiply(&matRot,&matRotNOx); break;
           case 5: newMatRot.Multiply(&matRot,&matRotNOy); break;
           case 6: newMatRot.Multiply(&matRot,&matRotNOz); break;
          }

          if( AIMoves[curAIMove].tx || AIMoves[curAIMove].ty || AIMoves[curAIMove].tz ) {
            xPos += AIMoves[curAIMove].tx;
            yPos += AIMoves[curAIMove].ty;
            zPos += AIMoves[curAIMove].tz;
            InitTranslate();
          }

          curAIMove++;

        }

        lastAIMoveTime = curTime;

      }
    } break;

  }

  if( !(gameMode==GAME_PLAYING || gameMode==GAME_DEMO) )
    return exitValue;

  // Compute rotation
  tmpRotation.Identity();

  if( rotateMode ) {
    float rTime = (fTime - startRotateTime);
    if( rTime > animationTime ) {
      // End of rotation
      matRot = newMatRot;
      rotateMode = 0;
    } else {

      float angle;
      if( rotateMode>0 ) {
        angle = PI/2.0f * (rTime/animationTime);
      } else {
        // Blocked rotation: little shake
        angle = PI/5.0f * sinf( (rTime/animationTime) * PI );
      }

      switch(rotateMode) {
        case 1: case -1: tmpRotation.RotateX(angle);  break;
        case 2: case -2: tmpRotation.RotateY(-angle); break;
        case 3: case -3: tmpRotation.RotateZ(angle);  break;
        case 4: case -4: tmpRotation.RotateX(-angle); break;
        case 5: case -5: tmpRotation.RotateY(angle);  break;
        case 6: case -6: tmpRotation.RotateZ(-angle); break;
      }
    }
  }

  // Compute translation
  float tTime = (fTime - startTranslateTime);

  if( tTime > animationTime ) {
    // End of translation
    vPos.x = (float)xPos;
    vPos.y = (float)yPos;
    vPos.z = (float)zPos;
  } else {
    vPos.x = vOrgPos.x + (vTransPos.x * (tTime/animationTime));
    vPos.y = vOrgPos.y + (vTransPos.y * (tTime/animationTime));
    vPos.z = vOrgPos.z + (vTransPos.z * (tTime/animationTime));
  }

  // Compute drop
  if( dropMode ) {
    float dTime = (fTime - startTranslateTime);
    startStepTime = fTime; // No step while drop
    if( dTime > dropTime ) {
      // End of drop
      vPos.z = (float)zPos;
      dropMode = false;
      // Process last key
      if( !lastKeyMode ) {
        redMode = true;
        startRedTime = fTime;
      } else {
        // Fast step on drop (100 ms)
        startStepTime = fTime - stepTime + 0.1f;
        lastKeyMode = false;
      }
    } else {
      vPos.x = vOrgPos.x + (vTransPos.x * (dTime/dropTime));
      vPos.y = vOrgPos.y + (vTransPos.y * (dTime/dropTime));
      vPos.z = vOrgPos.z + (vTransPos.z * (dTime/dropTime));
    }
  }

  // Step
  if( !practiceFlag && !redMode && (gameMode==GAME_PLAYING) ) {
    if( startStepTime==0.0f ) {
      // First step
      startStepTime = fTime;
    } else {
      float sTime = (fTime - startStepTime);
      if( sTime > stepTime ) {
        startStepTime = fTime;
        cursorPos++;
        dropPos--;
        if( IsLower() ) {
          if( !IsOverlap(&newMatRot,xPos,yPos,zPos+1) ) {
            zPos++;
            InitTranslate();
          } else {
            redMode = true;
            startRedTime = fTime;
          }
        }
      }
    }
  }

  // Spark
  if( startSpark != 0.0f && (fTime - startSpark) >= sparkTime )
    startSpark = 0.0f;

  // Red mode (end of move)
  if( redMode ) {
    float rTime = (fTime - startRedTime);
    if( rTime > 0.05f ) {
      AddPolyCube();
      NewPolyCube(keys);
    }
  }

  if( gameMode==GAME_OVER ) {
    // Start pit animation
    startEndTime = fTime + 0.5f;
  }

  return exitValue;

}

// ---------------------------------------------------------------------

void GameLogic::AddPolyCube() {

  // Add the polycube to the pit
  BLOCKITEM cubes[MAX_CUBE];
  int nbCube;
  allPolyCube[pIdx].CopyCube(cubes,&nbCube);
  TransformCube(&newMatRot,cubes,nbCube,xPos,yPos,zPos);

  for(int i=0;i<nbCube;i++)
    thePit.AddCube( cubes[i].x , cubes[i].y , cubes[i].z );

  // Remove lines
  int nbLines = thePit.RemoveLines();
  bool pitEmpty = thePit.IsEmpty();
  if( nbLines > 0 ) {
    switch( config.soundType ) {
      case SOUND_BLOCKOUT2: Play( pitEmpty ? Sound::Empty : Sound::Line ); break;
      case SOUND_BLOCKOUT:  Play( pitEmpty ? Sound::Empty2 : Sound::Line2 ); break;
    }
  }

  // Update score and level
  ComputeScore(nbLines,pitEmpty);
  if( level<10 ) {
    if( score.nbCube >= cubePerLevel * (level+1) ) {
      level++;
      stepTime *= timeLevelFactor;
      switch( config.soundType ) {
        case SOUND_BLOCKOUT2: Play(Sound::Level); break;
        case SOUND_BLOCKOUT:  Play(Sound::Level2); break;
      }
    }
  }

}

// ---------------------------------------------------------------------

void GameLogic::ComputeScore(int nbLines,bool pitEmpty) {

  if( !dropped )  dropPos = 0;
  if( dropPos<0 ) dropPos = 0;

  // PolyCube score
  float fPos   = (float)dropPos / (float)(thePit.GetDepth()-1);
  float lScore = (float)allPolyCube[pIdx].GetLowScore();
  float hScore = (float)allPolyCube[pIdx].GetHighScore();
  float pScore = (lScore + (hScore-lScore) * fPos) * pLevelFactor[level];

  // Line score
  float lineScore = lineBase[ config.blockSet ] * lLeveFactor[level] * lNumberFactor[nbLines];

  // Empty pit ( 2 lines bonus )
  float pitScore = 0.0f;
  if( pitEmpty )
    pitScore = lineBase[ config.blockSet ] * lLeveFactor[level] * lNumberFactor[2];

  // Total score
  float totalScore = ( lineScore + pScore + pitScore ) * depthFactor[ thePit.GetDepth() ];

  int iScore = fround( totalScore );
  if( iScore<1 ) iScore = 1;

  score.score  += iScore;
  score.nbCube += allPolyCube[pIdx].GetNbCube();
  if(pitEmpty) score.emptyPit++;

  switch( nbLines ) {
    case 1: score.nbLine1++; break;
    case 2: score.nbLine2++; break;
    case 3: score.nbLine3++; break;
    case 4: score.nbLine4++; break;
    case 5: score.nbLine5++; break;
  }

}

// ---------------------------------------------------------------------

void GameLogic::NewPolyCube(byte *keys) {

  // Init position and rotation
  vPos = v( 0.0f , 0.0f , 0.0f );
  xPos = 0;
  yPos = 0;
  zPos = 0;
  matRot.Identity();
  newMatRot.Identity();

  // Reset keys and mode
  if( keys )
    memset( keys, 0, KEY_COUNT );
  startRotateTime = 0.0f;
  startTranslateTime = 0.0f;
  rotateMode = 0;
  dropMode = false;
  lastKeyMode = false;
  redMode = false;
  startStepTime = 0.0f;
  dropPos = thePit.GetDepth() - 1;
  cursorPos = 0;
  dropped = false;
  lastAIMoveTime = curTime;
  curAIMove = 0;

  // Select new polycube
  pIdx = SelectPolyCube();
  if( gameMode==GAME_DEMO )
    botPlayer.GetMoves(&thePit,&(allPolyCube[pIdx]),xPos,yPos,zPos,AIMoves,&nbAIMove);

}

// ---------------------------------------------------------------------

void GameLogic::StartDrop() {

  if( gameMode==GAME_PAUSED || gameMode==GAME_OVER ) return;

  if( !dropMode && !redMode && !lastKeyMode ) {

    dropMode = true;
    dropped = true;

    // Compute translation
    int z = zPos;
    while( !IsOverlap(&newMatRot,xPos,yPos,z+1) ) z++;
    zPos = z;
    // Position the cursor to the bottom of the block
    cursorPos = GetBottom();
    InitTranslate();

  }

}

// ---------------------------------------------------------------------

void GameLogic::InitTranslate() {

  startTranslateTime = curTime;
  vOrgPos = vPos;
  vTransPos = v( (float)xPos - vPos.x ,
                 (float)yPos - vPos.y ,
                 (float)zPos - vPos.z );

}

// ---------------------------------------------------------------------

bool GameLogic::StartTranslate(int tType) {

  if( gameMode==GAME_PAUSED || gameMode==GAME_OVER ) return true;

  if( dropMode || redMode ) {
    if( dropMode ) lastKeyMode = true;
    return false;
  }

  // Direction of each translation type (1..8)
  static const int dirX[] = { 0, 1,-1, 0, 0, 1,-1, 1,-1 };
  static const int dirY[] = { 0, 0, 0, 1,-1, 1, 1,-1,-1 };
  int dx = dirX[tType];
  int dy = dirY[tType];

  if( !IsOverlap(&newMatRot,xPos+dx,yPos+dy,zPos) ) {
    xPos += dx; yPos += dy;
    InitTranslate();
  } else if( dx && dy ) {
    // Diagonal move blocked: try each axis alone
    if( !IsOverlap(&newMatRot,xPos+dx,yPos,zPos) ) {
      xPos += dx;
      InitTranslate();
    } else if( !IsOverlap(&newMatRot,xPos,yPos+dy,zPos) ) {
      yPos += dy;
      InitTranslate();
    }
  }

  return true;

}

// ---------------------------------------------------------------------

void GameLogic::StartSpark(const BLOCKITEM *pos) {

  Play(Sound::Hit);
  startSpark = curTime;
  sparkPos = *pos;

}

// ---------------------------------------------------------------------

bool GameLogic::StartRotate(int rType) {

  if( gameMode==GAME_PAUSED || gameMode==GAME_OVER ) return true;

  if( (rotateMode<=0) && !dropMode && !redMode ) {

    rotateMode = rType;
    startRotateTime = curTime;

    // Compute new rotation matrix
    switch(rotateMode) {
     case 1: newMatRot.Multiply(&matRot,&matRotOx); break;
     case 2: newMatRot.Multiply(&matRot,&matRotOy); break;
     case 3: newMatRot.Multiply(&matRot,&matRotOz); break;
     case 4: newMatRot.Multiply(&matRot,&matRotNOx); break;
     case 5: newMatRot.Multiply(&matRot,&matRotNOy); break;
     case 6: newMatRot.Multiply(&matRot,&matRotNOz); break;
    }

    // Check overlap
    int tx,ty,tz;
    BLOCKITEM pos;
    if( IsOverlap(&newMatRot,xPos,yPos,zPos,&tx,&ty,&tz,&pos) ) {
      // Check if the rotation can be done by a combination with a translation
      if( IsOverlap(&newMatRot,xPos+tx,yPos+ty,zPos+tz) ) {
        // Abort rotation
        rotateMode = -rotateMode;
        newMatRot = matRot;
        StartSpark(&pos);
      } else {
        // Combine rotation and translation
        xPos += tx;
        yPos += ty;
        zPos += tz;
        InitTranslate();
      }
    }

    return true;

  } else {

    if( dropMode )
      lastKeyMode = true;
    return false;

  }

}

// ---------------------------------------------------------------------

int GameLogic::GetBottom() const {

  int max = 0;

  BLOCKITEM cubes[MAX_CUBE];
  int nbCube;
  allPolyCube[pIdx].CopyCube(cubes,&nbCube);
  TransformCube(&newMatRot,cubes,nbCube,xPos,yPos,zPos);

  for(int i=0;i<nbCube;i++)
    if(cubes[i].z >= max) max=cubes[i].z;

  return max;

}

// ---------------------------------------------------------------------

bool GameLogic::IsLower() const {

  bool upper = false;

  BLOCKITEM cubes[MAX_CUBE];
  int nbCube;
  allPolyCube[pIdx].CopyCube(cubes,&nbCube);
  TransformCube(&newMatRot,cubes,nbCube,xPos,yPos,zPos);

  // Check if all cubes are above the descent cursor.
  // Note: BlockOut II advances i twice per iteration here, so only every
  // second cube is tested. Kept as is: it is part of the game feel.
  for(int i=0;i<nbCube && !upper;i++) {
    upper = (cubes[i].z >= cursorPos);
    if(!upper) i++;
  }

  return !upper;

}

// ---------------------------------------------------------------------

bool GameLogic::IsOverlap(const Matrix *mRot,int tx,int ty,int tz,int *ox,int *oy,int *oz,BLOCKITEM *pos) const {

  bool overlap = false;
  int mox = 0, moy = 0, moz = 0;
  int lox = 0, loy = 0, loz = 0;
  if( pos ) memset(pos,0,sizeof(BLOCKITEM));

  BLOCKITEM cubes[MAX_CUBE];
  int nbCube;
  allPolyCube[pIdx].CopyCube(cubes,&nbCube);
  TransformCube(mRot,cubes,nbCube,tx,ty,tz);

  // Check overlap between the polycube and the pit
  for(int i=0;i<nbCube;i++) {
    if( thePit.GetValue( cubes[i].x , cubes[i].y , cubes[i].z ) ) {
      if( pos ) *pos = cubes[i];
      overlap = true;
    }
    thePit.GetOutOfBounds( cubes[i].x , cubes[i].y , cubes[i].z , &lox , &loy , &loz );
    if( abs(lox) > abs(mox) ) mox = lox;
    if( abs(loy) > abs(moy) ) moy = loy;
    if( abs(loz) > abs(moz) ) moz = loz;
  }

  if( ox ) *ox = mox;
  if( oy ) *oy = moy;
  if( oz ) *oz = moz;

  return overlap;

}

//-----------------------------------------------------------------------------

void GameLogic::TransformCube(const Matrix *mRot,BLOCKITEM *cubes,int nbCube,
                              int tx,int ty,int tz) const {

  BLOCKITEM center = allPolyCube[pIdx].GetICenter();
  for(int i=0;i<nbCube;i++) {

    float rx,ry,rz,rw;
    mRot->TransfomVec( (float)(cubes[i].x - center.x) + 0.5f,
                       (float)(cubes[i].y - center.y) + 0.5f,
                       (float)(cubes[i].z - center.z) + 0.5f,
                       1.0f, &rx,&ry,&rz,&rw);

    cubes[i].x = fround( rx - 0.5f ) + center.x + tx ;
    cubes[i].y = fround( ry - 0.5f ) + center.y + ty ;
    cubes[i].z = fround( rz - 0.5f ) + center.z + tz ;

  }

}

//-----------------------------------------------------------------------------

int GameLogic::SelectPolyCube() {

  pIdx = possible[rng.Next() % nbPossible];

  // Translate the polycube to the left
  if( allPolyCube[pIdx].GetNbCube()>0 ) {
    xPos += thePit.GetWidth() - allPolyCube[pIdx].GetWidth();
    vPos.x = (float)xPos;
  }

  // Check game over state
  if( IsOverlap(&newMatRot,xPos,yPos,zPos) ) {
    score.gameTime = curTime - startGameTime;
    gameMode = GAME_OVER;
  }

  return pIdx;

}

//-----------------------------------------------------------------------------

void GameLogic::ComputeHelp() {

  int tx = thePit.GetWidth() - allPolyCube[pIdx].GetWidth();
  int ty = 0;
  int tz = 0;
  botPlayer.GetMoves(&thePit,&(allPolyCube[pIdx]),tx,ty,tz,AIMoves,&nbAIMove);

  Matrix aiRot;

  for(int i=nbAIMove-1;i>=0;i--) {

    switch( AIMoves[i].rotate ) {
       case 1: aiRot.Multiply(&matRotOx); break;
       case 2: aiRot.Multiply(&matRotOy); break;
       case 3: aiRot.Multiply(&matRotOz); break;
       case 4: aiRot.Multiply(&matRotNOx); break;
       case 5: aiRot.Multiply(&matRotNOy); break;
       case 6: aiRot.Multiply(&matRotNOz); break;
    }
    tx += AIMoves[i].tx;
    ty += AIMoves[i].ty;
    tz += AIMoves[i].tz;

  }

  // Drop
  while( !IsOverlap(&aiRot,tx,ty,tz+1) ) tz++;

  helpRot = aiRot;
  helpPos = BLOCKITEM{tx,ty,tz};
  startShowAI = curTime;

}

//-----------------------------------------------------------------------------

void GameLogic::HandleKey(byte *keys) {

  if( keys['P'] || keys['p'] ) {
    if( gameMode==GAME_PLAYING ) {
      gameMode = GAME_PAUSED;
      startPauseTime = curTime;
    } else if ( gameMode==GAME_PAUSED ) {
      gameMode = GAME_PLAYING;
      // Update time stamps
      startRotateTime    += (curTime - startPauseTime);
      startTranslateTime += (curTime - startPauseTime);
      startRedTime       += (curTime - startPauseTime);
      startStepTime      += (curTime - startPauseTime);
      startGameTime      += (curTime - startPauseTime);
    }
    keys['P']=0;
    keys['p']=0;
  }

  if( keys[KEY_RETURN] ) {
    if( gameMode==GAME_OVER )
      exitValue = (demoFlag || practiceFlag) ? 2 : 1;
    keys[KEY_RETURN]=0;
  }

  if( keys[KEY_ESCAPE] ) {
    // Abort the game and return to menu
    if( gameMode==GAME_PLAYING ) {
      score.gameTime = curTime - startGameTime;
      gameMode=GAME_OVER;
    }
    exitValue = (demoFlag || practiceFlag) ? 2 : 1;
    keys[KEY_ESCAPE]=0;
  }

  // Translations: cursor keys or numeric keypad (keypad digits)
  static const struct { int key; int digit; int type; } moves[] = {
    { KEY_UP, '8', 3 }, { KEY_DOWN, '2', 4 }, { KEY_LEFT, '4', 1 }, { KEY_RIGHT, '6', 2 },
    { KEY_HOME, '7', 5 }, { KEY_PAGEUP, '9', 6 }, { KEY_END, '1', 7 }, { KEY_PAGEDOWN, '3', 8 }
  };
  for(const auto &m : moves) {
    if( keys[m.key] || keys[m.digit] ) {
      if( StartTranslate(m.type) ) {
        keys[m.key]=0;
        keys[m.digit]=0;
      }
    }
  }

  if( keys[KEY_SPACE] ) {
    StartDrop();
    keys[KEY_SPACE]=0;
  }

  // Rotations (same order as BlockOut II: Rz2,Ry2,Rx2,Rz1,Ry1,Rx1)
  const struct { int key; int type; } rotations[] = {
    { config.keyRz2, 3 }, { config.keyRy2, 2 }, { config.keyRx2, 1 },
    { config.keyRz1, 6 }, { config.keyRy1, 5 }, { config.keyRx1, 4 }
  };
  for(const auto &r : rotations) {
    int lower = tolower(r.key);
    if( keys[r.key] || keys[lower] ) {
      if( StartRotate(r.type) ) {
        keys[r.key]=0;
        keys[lower]=0;
      }
    }
  }

  // Help mode
  if( (keys['H'] || keys['h']) && practiceFlag ) {
    ComputeHelp();
    keys['H']=0;
    keys['h']=0;
  }

}

} // namespace bo3
