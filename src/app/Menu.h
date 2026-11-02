/*
  File:        Menu.h
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
#pragma once

#include <string>
#include <vector>
#include "Hud.h"
#include "Scene.h"
#include "Settings.h"

namespace bo3 {

enum class MenuAction { None, StartGame, Demo, Practice, Quit, ToggleFullScreen, SettingsChanged };

// A part of the 3D menu scene: vertices [first, first+count) with their transform
struct MenuDraw {
  size_t first, count;
  Matrix modelView;
};

class Menu {

  public:

    Menu(Settings &settings,HighScores &scores);

    void SetSoundSink(SoundSink *sink) { sound = sink; }

    // One frame of menu logic
    MenuAction Process(byte *keys,float time);

    // After a game: hall of fame of the current setup, name entry when the
    // game made it into the top 10 (rank < 10)
    void ShowHallOfFame(int rank);

    // Back to the main page (after a demo or a practice)
    void ShowMain() { ToPage(Page::Main); }

    void Build3D(std::vector<Vertex> &out,std::vector<MenuDraw> &draws) const;
    void BuildUi(int canvasW,int canvasH,const FontInfo &font,std::vector<UiVertex> &out) const;

  private:

    enum class Page { Main, StartGame, ChooseSetup, ChangeSetup, HallOfFame, ScoreDetails,
                      Options, Graphics, Effects, Controls, Credits };

    struct Line { int x, y; bool selected; std::string text; };

    void ToPage(Page p,int sel = 0);
    void Play(Sound s) const { if( sound ) sound->Play(s); }
    void Animate(float time);
    MenuAction ProcessPage(byte *keys,float time);
    void PageLines(std::vector<Line> &lines,std::string &title,float time) const;
    std::string SetupName() const;
    bool Changed(int dir);   // left/right on an option line

    Settings   &settings;
    HighScores &scores;
    SoundSink  *sound = nullptr;

    Page  page = Page::Main;
    int   sel = 0;
    int   nbItem = 0;

    // Hall of fame name entry / key redefinition
    bool  editing = false;
    int   editRank = 0;
    char  editText[11] = {};
    int   editPos = 0;
    float editStart = 0.0f;
    int   scoreRank = 0;            // score shown on the details page
    SCOREREC detail{};

    // Intro: the letters of "BLOCKOUT III" fly into the grid
    static const int NB_LETTERS = 9;
    PolyCube letters[NB_LETTERS];
    Matrix   letterMatrix[NB_LETTERS];
    bool     letterAdded[NB_LETTERS] = {};
    bool     letterStarted[NB_LETTERS] = {};
    std::vector<uint8_t> grid;      // GRID_W x GRID_H cubes
    float    startTime = 0.0f;
    float    now = 0.0f;           // time of the last Process()
    bool     animEnded = false;
    bool     animSkipped = false;
};

} // namespace bo3
