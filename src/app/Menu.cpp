/*
  File:        Menu.cpp
  Description: Menu pages, intro animation and their drawing (no D3D)
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS (Menu*.cpp, Page*.cpp)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "Menu.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace bo3 {

namespace {

const int   GRID_W = 70, GRID_H = 50;
const float GRID_SIDE = 0.1f;
const float ANIM_TIME = 1.5f;     // flight of one letter
const float LETTER_DELAY = 0.25f; // between two letters

// Letters of "BLOCK" "OUT" "III" in cube coordinates (BlockOut II, last one widened)
const std::vector<std::pair<int,int>> LETTER_CUBES[9] = {
  { {0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{0,1},{4,1},{0,2},{4,2},{0,3},{1,3},{2,3},{3,3},{4,3},{1,4},{4,4},{1,5},{4,5},{1,6},{2,6},{3,6},{4,6},{5,6} },
  { {0,0},{1,0},{2,0},{2,1},{2,2},{2,3},{2,4} },
  { {0,0},{1,0},{2,0},{0,1},{2,1},{0,2},{2,2},{0,3},{2,3},{0,4},{1,4},{2,4} },
  { {0,0},{1,0},{2,0},{0,1},{2,1},{2,2},{0,3},{2,3},{0,4},{1,4},{2,4} },
  { {0,0},{1,0},{4,0},{5,0},{1,1},{4,1},{2,2},{4,2},{3,3},{4,3},{2,4},{4,4},{1,5},{4,5},{0,6},{1,6},{4,6},{5,6} },
  { {0,0},{1,0},{2,0},{0,1},{2,1},{0,2},{2,2},{0,3},{2,3},{0,4},{1,4},{2,4} },
  { {0,0},{1,0},{2,0},{0,1},{2,1},{0,2},{2,2},{0,3},{2,3},{0,4},{2,4} },
  { {1,0},{1,1},{1,2},{1,3},{0,4},{1,4},{2,4} },
  { {0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{1,1},{3,1},{5,1},{1,2},{3,2},{5,2},
    {0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3} } };

const VERTEX LETTER_START[9] = { {0.7f,-0.5f,0}, {0.25f,-0.5f,0}, {0,-0.5f,0}, {-0.25f,-0.5f,0}, {-0.7f,-0.5f,0},
                                 {0.25f,-0.5f,0}, {0,-0.5f,0}, {-0.25f,-0.5f,0}, {-0.5f,-0.5f,0} };
const VERTEX LETTER_END[9]   = { {0.65f,0.9f,3}, {0.25f,1.1f,3}, {-0.15f,1.1f,3}, {-0.55f,1.1f,3}, {-1.25f,0.9f,3},
                                 {0.25f,0.4f,3}, {-0.15f,0.4f,3}, {-0.55f,0.4f,3}, {-1.45f,0.4f,3} };

const char *BLOCKSET_NAME[] = { "FLAT", "BASIC", "EXTENDED" };
const char *FX_PRESET_NAME[] = { "Off", "Clean", "CRT", "Arcade", "Custom" };

std::string Fmt(const char *f,...) {
  char buf[256];
  va_list a;
  va_start(a,f);
  vsnprintf(buf,sizeof(buf),f,a);
  va_end(a);
  return buf;
}

std::string DateText(uint32 date,bool withTime) {
  if( !date ) return withTime ? "" : "..........";
  time_t t = date;
  tm ts;
  if( localtime_s(&ts,&t) ) return "";
  return withTime ? Fmt("%02d-%02d-%04d %02d:%02d:%02d",ts.tm_mday,ts.tm_mon+1,ts.tm_year+1900,ts.tm_hour,ts.tm_min,ts.tm_sec)
                  : Fmt("%02d-%02d-%04d",ts.tm_mday,ts.tm_mon+1,ts.tm_year+1900);
}

// "S[...........]F" style gauge: value 0..10
std::string Gauge(char lo,int v,char hi) {
  std::string s = Fmt("%c[",lo);
  for(int i=0;i<=10;i++) s += i<=v ? '#' : '.';
  return s + "]" + hi;
}

int Wrap(int v,int lo,int hi) { return v<lo ? hi : (v>hi ? lo : v); }

} // namespace

//-----------------------------------------------------------------------------

Menu::Menu(Settings &s,HighScores &hs) : settings(s), scores(hs), grid(GRID_W*GRID_H,0) {

  for(int i=0;i<NB_LETTERS;i++) {
    for(auto [x,y] : LETTER_CUBES[i]) letters[i].AddCube(x,y,0);
    letters[i].InitRotationCenter();
  }
  ToPage(Page::Main);

}

//-----------------------------------------------------------------------------

void Menu::ToPage(Page p,int s) {

  page = p;
  sel = s;
  editing = false;
  static const int items[] = { 8, 10, 4, 5, 10, 1, 3, 6, 9, 8, 0 };
  nbItem = items[(int)p];
  Play(Sound::Blub);

}

void Menu::ShowHallOfFame(int rank) {

  ToPage(Page::HallOfFame,rank<10 ? rank : 0);
  if( rank<10 ) {
    editing = true;
    editRank = rank;
    memset(editText,' ',10);
    editText[10] = 0;
    editPos = 0;
    editStart = 0.0f;
  }

}

std::string Menu::SetupName() const {

  auto is = [&](int w,int h,int d,int set) {
    return ((settings.pitWidth==w && settings.pitHeight==h) || (settings.pitWidth==h && settings.pitHeight==w)) &&
           settings.pitDepth==d && settings.blockSet==set;
  };
  if( is(5,5,12,BLOCKSET_FLAT) )     return "[Flat Fun]";
  if( is(3,3,10,BLOCKSET_BASIC) )    return "[3D Mania]";
  if( is(5,5,10,BLOCKSET_EXTENDED) ) return "[Out of Control]";
  return Fmt("[%dx%dx%d,%s]",settings.pitWidth,settings.pitHeight,settings.pitDepth,BLOCKSET_NAME[settings.blockSet]);

}

//-----------------------------------------------------------------------------
// Intro (BlockOut II Menu::ProcessAnim)

void Menu::Animate(float time) {

  if( startTime==0.0f ) startTime = time;
  float t = time - startTime;
  Matrix org;
  org.Init33( 1,0,0, 0,-1,0, 0,0,-1 );

  for(int i=0;i<NB_LETTERS;i++) {
    float delay = i * LETTER_DELAY;
    float ratio = 0.0f;
    if( t > ANIM_TIME + delay ) {
      ratio = 1.0f;
      if( i==NB_LETTERS-1 ) animEnded = true;
      if( !letterAdded[i] ) {
        // The letter becomes cubes of the grid
        letterAdded[i] = true;
        int ox = fround(LETTER_END[i].x*10.0f + 0.5f) + GRID_W/2;
        int oy = fround(LETTER_END[i].y*10.0f) + GRID_H/2;
        for(auto [x,y] : LETTER_CUBES[i])
          if( ox+x>=0 && ox+x<GRID_W && oy+y>=0 && oy+y<GRID_H ) grid[(oy+y)*GRID_W + ox+x] = 1;
        if( !animSkipped ) Play(Sound::Tchh);
      }
    } else if( t > delay ) {
      if( (i==1 || i==5) && !letterStarted[i] ) { Play(Sound::Wozz); letterStarted[i] = true; }
      ratio = (t - delay) / ANIM_TIME;
    }
    const VERTEX &a = LETTER_START[i], &b = LETTER_END[i];
    Matrix r;
    r.RotateX(PI * ratio);
    letterMatrix[i] = Mul(Mul(Translation(a.x + (b.x-a.x)*ratio, a.y + (b.y-a.y)*ratio, a.z + (b.z-a.z)*ratio),r),org);
  }

}

//-----------------------------------------------------------------------------

MenuAction Menu::Process(byte *keys,float time) {

  now = time;
  Animate(time);
  if( !animEnded ) {
    // Escape ends the intro at once
    if( keys[KEY_ESCAPE] ) {
      startTime = time - (ANIM_TIME + NB_LETTERS*LETTER_DELAY);
      animSkipped = true;
      keys[KEY_ESCAPE] = 0;
    }
    return MenuAction::None;
  }
  return ProcessPage(keys,time);

}

// Left (-1) / right (+1) on the selected line of an option page
bool Menu::Changed(int dir) {

  Settings &s = settings;
  switch(page) {
    case Page::ChangeSetup:
      switch(sel) {
        case 0: s.pitWidth  = Wrap(s.pitWidth+dir,MIN_PITWIDTH,MAX_PITWIDTH); return true;
        case 1: s.pitHeight = Wrap(s.pitHeight+dir,MIN_PITHEIGHT,MAX_PITHEIGHT); return true;
        case 2: s.pitDepth  = Wrap(s.pitDepth+dir,MIN_PITDEPTH,MAX_PITDEPTH); return true;
        case 3: s.blockSet  = Wrap(s.blockSet+dir,BLOCKSET_FLAT,BLOCKSET_EXTENDED); return true;
      }
      break;
    case Page::Graphics:
      switch(sel) {
        case 0: s.playSound = !s.playSound; return true;
        case 2: s.animationSpeed = Wrap(s.animationSpeed+dir,0,10); return true;
        case 3: s.transparentFace = Wrap(s.transparentFace+dir,0,10); return true;
        case 4: s.soundType = Wrap(s.soundType+dir,SOUND_BLOCKOUT2,SOUND_BLOCKOUT); return true;
        case 5: s.lineWidth = Wrap(s.lineWidth+dir,0,10); return true;
      }
      break;
    case Page::Effects: {
      if( sel==0 ) {
        s.fxPreset = Wrap(s.fxPreset+dir,FX_OFF,FX_ARCADE);
        s.fx = FxFromPreset(s.fxPreset);
        return true;
      }
      bool *fx[] = { &s.fx.scanlines, &s.fx.vignette, &s.fx.glow, &s.fx.afterglow,
                     &s.fx.curvature, &s.fx.mask, &s.fx.ega, &s.fx.lowRes };
      *fx[sel-1] = !*fx[sel-1];
      s.fxPreset = FX_CUSTOM;
      return true;
    }
    default:
      break;
  }
  return false;

}

MenuAction Menu::ProcessPage(byte *keys,float time) {

  MenuAction action = MenuAction::None;
  auto take = [&](int k) { bool on = keys[k]!=0; keys[k] = 0; return on; };

  // Name entry after a game
  if( editing && page==Page::HallOfFame ) {
    if( editStart==0.0f ) editStart = time;
    for(int c=32;c<127;c++) {
      if( !keys[c] ) continue;
      keys[c] = 0;
      if( editPos<10 ) editText[editPos++] = (char)c;
    }
    if( take(KEY_DELETE) | take(KEY_BACKSPACE) | take(KEY_LEFT) ) {
      if( editPos>0 ) editPos--;
      editText[editPos] = ' ';
    }
    if( take(KEY_RETURN) | take(KEY_ESCAPE) ) {
      scores.SetName(settings.GetSetupId(),editRank,editText);
      editing = false;
      return MenuAction::SettingsChanged;   // saves the high scores
    }
    return MenuAction::None;
  }

  // Key redefinition: next letter
  if( editing && page==Page::Controls ) {
    if( take(KEY_ESCAPE) ) { editing = false; return action; }
    for(int c='A';c<='Z';c++) {
      if( keys[c] || keys[c+32] ) {
        keys[c] = keys[c+32] = 0;
        int *k[] = { &settings.keyRx1, &settings.keyRx2, &settings.keyRy1, &settings.keyRy2, &settings.keyRz1, &settings.keyRz2 };
        *k[sel] = c;
        editing = false;
        return MenuAction::SettingsChanged;
      }
    }
    return action;
  }

  if( nbItem>0 ) {
    if( take(KEY_DOWN) ) sel = (sel+1) % nbItem;
    if( take(KEY_UP) )   sel = (sel+nbItem-1) % nbItem;
  }
  int dir = take(KEY_LEFT) ? -1 : (take(KEY_RIGHT) ? 1 : 0);
  if( page==Page::Graphics && sel==1 && dir ) return MenuAction::ToggleFullScreen;
  if( dir && Changed(dir) ) {
    if( page==Page::Graphics && sel==0 ) Play(Sound::Blub);
    action = MenuAction::SettingsChanged;
  }
  bool enter = take(KEY_RETURN);
  bool escape = take(KEY_ESCAPE);

  switch(page) {

    case Page::Main:
      if( enter ) {
        switch(sel) {
          case 0: ToPage(Page::StartGame,settings.startLevel); break;
          case 1: ToPage(Page::ChooseSetup); break;
          case 2: ToPage(Page::HallOfFame); break;
          case 3: ToPage(Page::Options); break;
          case 4: return MenuAction::Demo;
          case 5: return MenuAction::Practice;
          case 6: ToPage(Page::Credits); break;
          case 7: return MenuAction::Quit;
        }
      }
      if( escape ) return MenuAction::Quit;
      break;

    case Page::StartGame:
      if( enter ) {
        settings.startLevel = sel;
        Play(Sound::Blub);
        return MenuAction::StartGame;
      }
      if( escape ) ToPage(Page::Main);
      break;

    case Page::ChooseSetup:
      if( enter ) {
        static const int preset[3][4] = { {5,5,12,BLOCKSET_FLAT}, {3,3,10,BLOCKSET_BASIC}, {5,5,10,BLOCKSET_EXTENDED} };
        if( sel<3 ) {
          settings.pitWidth = preset[sel][0]; settings.pitHeight = preset[sel][1];
          settings.pitDepth = preset[sel][2]; settings.blockSet = preset[sel][3];
          ToPage(Page::StartGame,settings.startLevel);
          return MenuAction::SettingsChanged;
        }
        ToPage(Page::ChangeSetup);
      }
      if( escape ) ToPage(Page::Main);
      break;

    case Page::ChangeSetup:
      if( enter ) {
        if( sel==4 ) ToPage(Page::StartGame,settings.startLevel);
        else if( Changed(1) ) action = MenuAction::SettingsChanged;
      }
      if( escape ) ToPage(Page::Main);
      break;

    case Page::HallOfFame:
      if( enter ) {
        SCOREREC top[10];
        scores.Top10(settings.GetSetupId(),top);
        detail = top[sel];
        scoreRank = sel;
        ToPage(Page::ScoreDetails);
      }
      if( escape ) ToPage(Page::Main);
      break;

    case Page::ScoreDetails:
      if( enter ) ToPage(Page::StartGame,settings.startLevel);
      if( escape ) ToPage(Page::HallOfFame,scoreRank);
      break;

    case Page::Options:
      if( enter ) ToPage(sel==0 ? Page::Controls : (sel==1 ? Page::Graphics : Page::Effects));
      if( escape ) ToPage(Page::Main);
      break;

    case Page::Graphics:
      if( enter ) {
        if( sel==1 ) return MenuAction::ToggleFullScreen;
        if( Changed(1) ) action = MenuAction::SettingsChanged;
      }
      if( escape ) ToPage(Page::Options,1);
      break;

    case Page::Effects:
      if( enter && Changed(1) ) action = MenuAction::SettingsChanged;
      if( escape ) ToPage(Page::Options,2);
      break;

    case Page::Controls:
      if( enter ) {
        if( sel<6 ) { editing = true; editStart = time; }
        else {
          static const char qwerty[] = "QAWSED", azerty[] = "AQZSED";   // x1 x2 y1 y2 z1 z2
          const char *k = sel==6 ? qwerty : azerty;
          settings.keyRx1 = k[0]; settings.keyRx2 = k[1]; settings.keyRy1 = k[2];
          settings.keyRy2 = k[3]; settings.keyRz1 = k[4]; settings.keyRz2 = k[5];
          action = MenuAction::SettingsChanged;
        }
      }
      if( escape ) ToPage(Page::Options,0);
      break;

    case Page::Credits:
      if( escape || enter ) ToPage(Page::Main,6);
      break;
  }
  return action;

}

//-----------------------------------------------------------------------------
// Page content (BlockOut II Page*::Render): text at column x, row y

void Menu::PageLines(std::vector<Line> &L,std::string &title,float time) const {

  auto add = [&](int x,int y,bool s,const std::string &t) { L.push_back(Line{ x,y,s,t }); };
  auto item = [&](int y,int idx,const std::string &t) { add(0,y,sel==idx,t); };
  const Settings &s = settings;

  switch(page) {

    case Page::Main: {
      title = "MAIN MENU";
      static const char *items[] = { "Start Game  ", "Choose Setup", "Hall of Fame", "Options     ",
                                     "Demo        ", "Practice    ", "Credits     ", "Quit        " };
      for(int i=0;i<8;i++) item(i,i,items[i]);
      add(13,0,false,SetupName());
      break;
    }

    case Page::StartGame:
      title = "STARTING LEVEL";
      for(int i=0;i<10;i++) add(14,i,sel==i,Fmt("Level %d",i));
      break;

    case Page::ChooseSetup:
      title = "CHOOSE SETUP";
      item(0,0,"Flat Fun      ");     add(15,0,false,"Pit:      5x5x12"); add(15,1,false,"Block Set:FLAT");
      item(3,1,"3D Mania      ");     add(15,3,false,"Pit:      3x3x10"); add(15,4,false,"Block Set:BASIC");
      item(6,2,"Out of Control");     add(15,6,false,"Pit:      5x5x10"); add(15,7,false,"Block Set:EXTENDED");
      item(9,3,"Change Setup  ");
      break;

    case Page::ChangeSetup:
      title = "CHANGE SETUP";
      item(0,0,"Pit Length :"); add(13,0,false,Fmt("%d",s.pitWidth));
      item(1,1,"Pit Width  :"); add(13,1,false,Fmt("%d",s.pitHeight));
      item(2,2,"Pit Depth  :"); add(13,2,false,Fmt("%d",s.pitDepth));
      item(3,3,"Block Set  :"); add(13,3,false,BLOCKSET_NAME[s.blockSet]);
      item(4,4,"Start Game  ");
      break;

    case Page::HallOfFame: {
      title = "HALL OF FAME " + SetupName();
      SCOREREC top[10];
      scores.Top10(s.GetSetupId(),top);
      for(int i=0;i<10;i++) {
        bool hl = sel==i && !editing;
        add(0,i,hl,Fmt("%2d ",i+1));
        add(3,i,hl,(editing && i==editRank) ? std::string(editText) : std::string(top[i].name));
        add(13,i,hl,Fmt("%7d ",top[i].score));
        add(21,i,hl,"[" + DateText(top[i].date,false) + "]");
      }
      if( editing ) {
        // Blinking cursor
        bool on = ((int)floorf((time - editStart) * 2.0f) % 2)==0;
        add(3+editPos,editRank,on," ");
      }
      break;
    }

    case Page::ScoreDetails: {
      const SCOREREC &d = detail;
      title = Fmt("SCORE DETAILS [Rank %d]",scoreRank+1);
      add(0,0,false,"Setup: " + SetupName());
      add(0,1,false,Fmt("Score: %d by %s",d.score,d.name));
      add(0,2,false,"Date : " + DateText(d.date,true));
      add(0,3,false,Fmt("Time : %dmin %02dsec",(int)d.gameTime/60,(int)d.gameTime%60));
      add(0,4,false,Fmt("Cubes: %d",d.nbCube));
      int perLevel = s.pitWidth*15 + s.pitHeight*15;
      int endLevel = d.nbCube / perLevel;
      std::string end = endLevel>=10 ? "#" : Fmt("%d",std::max(endLevel,d.startLevel));
      add(0,5,false,Fmt("Level: From %d To ",d.startLevel) + end);
      add(0,6,false,Fmt("Flush: %d",d.emptyPit));
      int lines = d.nbLine1 + d.nbLine2*2 + d.nbLine3*3 + d.nbLine4*4 + d.nbLine5*5;
      add(0,7,false,Fmt("Lines: %3d %02d[x1] %02d[x2] %02d[x3]",lines,d.nbLine1,d.nbLine2,d.nbLine3));
      add(0,8,false,Fmt("           %02d[x4] %02d[x5]",d.nbLine4,d.nbLine5));
      item(9,0,"Try to beat");
      break;
    }

    case Page::Options:
      title = "OPTIONS";
      item(0,0,"Controls          ");
      item(1,1,"Graphics & Sound  ");
      item(2,2,"Screen Effects    ");
      break;

    case Page::Graphics:
      title = "GRAPHICS & SOUND";
      item(0,0,"Sound            :"); add(19,0,false,s.playSound ? "Enabled" : "Disabled");
      item(1,1,"FullScreen       :"); add(19,1,false,s.fullScreen ? "Enabled" : "Disabled");
      item(2,2,"Rotation Speed   :"); add(19,2,false,Gauge('S',s.animationSpeed,'F'));
      item(3,3,"Transparent face :"); add(19,3,false,Gauge('T',s.transparentFace,'O'));
      item(4,4,"Sound preset     :"); add(19,4,false,s.soundType==SOUND_BLOCKOUT2 ? "Blockout II" : "Blockout");
      item(5,5,"Line width       :"); add(19,5,false,Gauge('T',s.lineWidth,'L'));
      break;

    case Page::Effects: {
      title = "SCREEN EFFECTS";
      item(0,0,"Preset           :"); add(19,0,false,FX_PRESET_NAME[s.fxPreset]);
      static const char *names[] = { "Scanlines        :", "Vignette         :", "Phosphor glow    :",
                                     "Afterglow        :", "Curvature        :", "Aperture mask    :",
                                     "EGA palette      :", "320x200 mode     :" };
      const bool on[] = { s.fx.scanlines, s.fx.vignette, s.fx.glow, s.fx.afterglow,
                          s.fx.curvature, s.fx.mask, s.fx.ega, s.fx.lowRes };
      for(int i=0;i<8;i++) { item(i+1,i+1,names[i]); add(19,i+1,false,on[i] ? "On" : "Off"); }
      break;
    }

    case Page::Controls: {
      title = "GAME CONTROLS";
      static const char *names[] = { "Rotate X     :", "Rotate X back:", "Rotate Y     :",
                                     "Rotate Y back:", "Rotate Z     :", "Rotate Z back:" };
      const int keys[] = { s.keyRx1, s.keyRx2, s.keyRy1, s.keyRy2, s.keyRz1, s.keyRz2 };
      for(int i=0;i<6;i++) {
        item(i,i,names[i]);
        bool blink = editing && sel==i;
        bool on = blink && ((int)floorf((time - editStart) * 2.0f) % 2)==0;
        add(15,i,false,"[ ]");
        add(16,i,on,blink ? std::string(" ") : std::string(1,(char)keys[i]));
      }
      item(6,6,"Reset to QWERTY");
      item(7,7,"Reset to AZERTY");
      if( !editing ) add(0,8,false,"[O]Sound on/off  [F1]Full screen");
      add(0,9,false,editing ? "Press new key or [Esc] to cancel" : "[Cursor]Move [Space]Drop [P]Pause");
      break;
    }

    case Page::Credits:
      title = "CREDITS";
      add(0,0,false,"Block Out III");
      add(0,1,false,"by Rekow IT, GNU GPL v3 or later");
      add(0,3,false,"Based on BlockOut II");
      add(0,4,false,"by Jean-Luc Pons");
      add(0,6,false,"Inspired by Blockout");
      add(0,7,false,"(California Dreams 1989)");
      add(0,9,false,"[Esc] Back");
      break;
  }

}

//-----------------------------------------------------------------------------
// Drawing

void Menu::Build3D(std::vector<Vertex> &out,std::vector<MenuDraw> &draws) const {

  Matrix view = GameView();
  const float c = GRID_SIDE;
  const float sx = -GRID_W*c/2.0f, sy = -GRID_H*c/2.0f, sz = 3.0f + c;

  // Grid lines and the cubes of the landed letters (BlockOut II MenuGrid)
  size_t first = out.size();
  auto line = [&](VERTEX a,VERTEX b,MaterialId m) {
    static const int corner[6][2] = { {0,1},{0,0},{1,1}, {1,1},{0,0},{1,0} };
    for(const auto &k : corner) {
      const VERTEX &p = k[0] ? b : a, &q = k[0] ? a : b;
      out.push_back(Vertex{ {p.x,p.y,p.z}, {q.x,q.y,q.z}, (uint32_t)m | (VK_LINE<<8) | (k[0]<<12) | (k[1]<<13) });
    }
  };
  for(int i=0;i<=GRID_W;i++) line(v(sx+i*c-c/2,-sy,sz),v(sx+i*c-c/2,sy,sz),MAT_MENU_GRID);
  for(int i=0;i<=GRID_H;i++) line(v(-sx-c/2,sy+i*c,sz),v(sx-c/2,sy+i*c,sz),MAT_MENU_GRID);

  // Cubes far from the centre first
  std::vector<std::pair<int,int>> cells;
  for(int y=0;y<GRID_H;y++)
    for(int x=0;x<GRID_W;x++)
      if( grid[y*GRID_W+x] ) cells.push_back({ x,y });
  auto dist = [](const std::pair<int,int> &p) {
    return (p.first-GRID_W/2)*(p.first-GRID_W/2) + (p.second-GRID_H/2)*(p.second-GRID_H/2); };
  std::stable_sort(cells.begin(),cells.end(),[&](auto &a,auto &b){ return dist(a) > dist(b); });

  for(auto [x,y] : cells) {
    VERTEX o = v(sx + x*c - c/2, sy + y*c, sz);
    auto P = [&](int a,int b,int d) { return v(o.x+a*c,o.y+b*c,o.z+d*c); };
    auto quad = [&](VERTEX a,VERTEX b,VERTEX cc,VERTEX d,VERTEX n,MaterialId m) {
      const VERTEX q[6] = { a,b,cc, a,cc,d };
      for(const VERTEX &p : q) out.push_back(Vertex{ {p.x,p.y,p.z}, {n.x,n.y,n.z}, (uint32_t)m | (VK_LIT<<8) });
    };
    quad(P(1,1,-1),P(1,1,0),P(1,0,0),P(1,0,-1),v(1,0,0),MAT_MENU_BLUE);
    quad(P(0,1,0),P(1,1,0),P(1,1,-1),P(0,1,-1),v(0,1,0),MAT_MENU_BLUE);
    quad(P(0,1,0),P(0,1,-1),P(0,0,-1),P(0,0,0),v(-1,0,0),MAT_MENU_BLUE);
    quad(P(1,0,-1),P(1,0,0),P(0,0,0),P(0,0,-1),v(0,-1,0),MAT_MENU_BLUE);
    quad(P(0,1,-1),P(1,1,-1),P(1,0,-1),P(0,0,-1),v(0,0,-1),MAT_MENU_RED);
    // Front square, then the edges facing the screen centre
    line(P(0,1,-1),P(1,1,-1),MAT_MENU_GRID); line(P(1,1,-1),P(1,0,-1),MAT_MENU_GRID);
    line(P(1,0,-1),P(0,0,-1),MAT_MENU_GRID); line(P(0,0,-1),P(0,1,-1),MAT_MENU_GRID);
    int mx = GRID_W/2, my = GRID_H/2;
    if( x<mx || y<my ) line(P(1,1,-1),P(1,1,0),MAT_MENU_GRID);
    if( x>mx || y<my ) line(P(0,1,-1),P(0,1,0),MAT_MENU_GRID);
    if( x<mx || y>my ) line(P(1,0,-1),P(1,0,0),MAT_MENU_GRID);
    if( x>mx || y>my ) line(P(0,0,-1),P(0,0,0),MAT_MENU_GRID);
    if( x>mx ) line(P(0,0,0),P(0,1,0),MAT_MENU_GRID); else line(P(1,1,0),P(1,0,0),MAT_MENU_GRID);
    if( y>my ) line(P(1,0,0),P(0,0,0),MAT_MENU_GRID); else line(P(0,1,0),P(1,1,0),MAT_MENU_GRID);
  }
  draws.push_back(MenuDraw{ first, out.size()-first, view });

  // Flying letters (white outlines)
  PitGeometry g{};
  g.cubeSide = GRID_SIDE;
  for(int i=0;i<NB_LETTERS;i++) {
    if( letterAdded[i] ) continue;
    first = out.size();
    BuildBlockLines(letters[i],g,MAT_WHITE,out);
    draws.push_back(MenuDraw{ first, out.size()-first, Mul(view,letterMatrix[i]) });
  }

}

void Menu::BuildUi(int canvasW,int canvasH,const FontInfo &font,std::vector<UiVertex> &out) const {

  if( !animEnded ) return;

  UiBuilder ui(out,font);
  const float W = (float)canvasW, H = (float)canvasH;
  const float t = std::max(1.0f,floorf(H/384.0f));
  const uint32_t BLUE = 0xFF800000, CYAN = 0xFFFFFF00, YELLOW = 0xFF00FFFF, WHITE = 0xFFFFFFFF, GRAY = 0xFF555555;   // EGA dark grey: readable with every effect

  // Panel: dark blue, double cyan frame (BlockOut II menuback.png)
  float px = floorf(W*0.13f), py = floorf(H*0.41f), pw = floorf(W*0.87f) - px, ph = floorf(H*0.985f) - py;
  ui.Rect(px,py,pw,ph,BLUE);
  ui.Frame(px+t,py+t,pw-2*t,ph-2*t,t,CYAN);
  ui.Frame(px+3*t,py+3*t,pw-6*t,ph-6*t,t,CYAN);

  // Text grid of BlockOut II: column width 0.0205 W, line height 0.045 H.
  // Integer pixel scale, with the tall pixels of an EGA text screen.
  const int sx = std::max(1,(int)(W*0.0205f / font.glyphW));
  const int sy = std::max(1,(int)(sx*1.5f));
  const float cw = (float)(font.glyphW*sx);

  std::vector<Line> lines;
  std::string title;
  // Blinking cursors use the clock of the last processed frame
  PageLines(lines,title,now);

  auto text = [&](float x,float y,bool selected,const std::string &s) {
    if( selected ) ui.Rect(x,y,cw*s.size(),(float)(font.glyphH*sy),GRAY);
    ui.Text(x,y,sx,s.c_str(),selected ? WHITE : YELLOW,sy);
  };
  text(floorf((W - cw*title.size())/2),floorf(H*0.43f),false,title);
  for(const Line &l : lines)
    text(floorf(W*0.15f) + l.x*cw, floorf(H*(0.515f + l.y*0.045f)), l.selected, l.text);

}

} // namespace bo3
