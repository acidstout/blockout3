/*
  File:        main.cpp
  Description: Win32 application: window, input, timing, main loop
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
#include <windows.h>
#include <cstdio>
#include <ctime>
#include <memory>
#ifdef BO3_PERF
#include <algorithm>
#include <vector>
#endif
#include "GameLogic.h"
#include "Audio.h"
#include "Menu.h"
#include "Renderer.h"
#include "Storage.h"

using namespace bo3;

namespace {

HWND            g_wnd;
byte            g_keys[KEY_COUNT];
Settings        g_settings;
HighScores      g_scores;
WINDOWPLACEMENT g_placement = { sizeof(WINDOWPLACEMENT) };   // windowed placement
Renderer        g_renderer;
Audio           g_audio;

const DWORD WINDOWED_STYLE   = WS_OVERLAPPEDWINDOW;
const DWORD FULLSCREEN_STYLE = WS_POPUP;

//-----------------------------------------------------------------------------
// Borderless fullscreen on the monitor that holds the window

// Covers the whole monitor (again after a resolution or monitor change)
void FitToMonitor() {

  MONITORINFO mi = { sizeof(mi) };
  GetMonitorInfoW(MonitorFromWindow(g_wnd,MONITOR_DEFAULTTONEAREST),&mi);
  const RECT &r = mi.rcMonitor;
  SetWindowPos(g_wnd,HWND_TOP,r.left,r.top,r.right-r.left,r.bottom-r.top,
               SWP_FRAMECHANGED|SWP_NOOWNERZORDER);

}

void SetFullScreen(bool full) {

  if( full ) {
    GetWindowPlacement(g_wnd,&g_placement);
    SetWindowLongPtrW(g_wnd,GWL_STYLE,FULLSCREEN_STYLE|WS_VISIBLE);
    FitToMonitor();
  } else {
    SetWindowLongPtrW(g_wnd,GWL_STYLE,WINDOWED_STYLE|WS_VISIBLE);
    SetWindowPlacement(g_wnd,&g_placement);
    SetWindowPos(g_wnd,nullptr,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
  }
  g_settings.fullScreen = full;
  ShowCursor(!full);

}

//-----------------------------------------------------------------------------
// Keys are stored like BlockOut II (SDL 1.2 codes): printable keys by their
// character, the others by their key code.

int KeyCode(WPARAM vk) {

  switch(vk) {
    case VK_UP:     return KEY_UP;
    case VK_DOWN:   return KEY_DOWN;
    case VK_LEFT:   return KEY_LEFT;
    case VK_RIGHT:  return KEY_RIGHT;
    case VK_INSERT: return KEY_INSERT;
    case VK_HOME:   return KEY_HOME;
    case VK_END:    return KEY_END;
    case VK_DELETE: return KEY_DELETE;
    case VK_PRIOR:  return KEY_PAGEUP;
    case VK_NEXT:   return KEY_PAGEDOWN;
  }
  return 0;

}

LRESULT CALLBACK WndProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp) {

  switch(msg) {

    case WM_CHAR:
      // Characters, Return, Escape, Space (Ctrl+letter control codes are ignored)
      if( wp<128 && (wp>=32 || wp==VK_RETURN || wp==VK_ESCAPE || wp==VK_BACK) )
        g_keys[wp] = 1;
      return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if( wp==VK_F1 || (wp==VK_RETURN && (lp & (1<<29))) ) {    // F1 or Alt+Enter
        if( !(lp & (1<<30)) ) SetFullScreen(!g_settings.fullScreen);
        return 0;
      }
      if( int k = KeyCode(wp) ) { g_keys[k] = 1; return 0; }
      break;

    case WM_SYSCHAR:
      return 0;   // no beep on Alt+Enter

    case WM_SIZE:
      g_renderer.Resize();
      return 0;

    case WM_GETMINMAXINFO:
      ((MINMAXINFO *)lp)->ptMinTrackSize = { 320, 240 };
      return 0;

    case WM_DPICHANGED: {
      // Moving to a monitor with another scale: take the size Windows proposes
      const RECT *r = (const RECT *)lp;
      if( g_settings.fullScreen ) FitToMonitor();
      else SetWindowPos(wnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,
                        SWP_NOZORDER|SWP_NOACTIVATE);
      return 0;
    }

    case WM_DISPLAYCHANGE:
      if( g_settings.fullScreen ) FitToMonitor();
      return 0;

    case WM_CLOSE:
      if( !g_settings.fullScreen ) GetWindowPlacement(wnd,&g_placement);
      DestroyWindow(wnd);
      return 0;

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

  }
  return DefWindowProcW(wnd,msg,wp,lp);

}

//-----------------------------------------------------------------------------

bool CreateMainWindow(HINSTANCE inst) {

  WNDCLASSEXW wc = { sizeof(wc) };
  wc.lpfnWndProc = WndProc;
  wc.hInstance = inst;
  wc.hIcon = LoadIconW(inst,MAKEINTRESOURCEW(1));
  wc.hCursor = LoadCursorW(nullptr,IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName = L"BlockOut3";
  if( !RegisterClassExW(&wc) ) return false;

  // Default: 4:3 window covering 75% of the primary monitor work area height
  RECT work;
  SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
  int h = (work.bottom-work.top) * 3 / 4;
  int w = h * 4 / 3;

  g_wnd = CreateWindowExW(0,wc.lpszClassName,L"Block Out III",WINDOWED_STYLE,
                          work.left + (work.right-work.left-w)/2, work.top + (work.bottom-work.top-h)/2,
                          w,h,nullptr,nullptr,inst,nullptr);
  if( !g_wnd ) return false;

  // Saved position only when its title bar is on a monitor (one may have been unplugged)
  RECT title = g_placement.rcNormalPosition;
  title.bottom = title.top + GetSystemMetrics(SM_CYCAPTION);
  if( g_placement.length && MonitorFromRect(&title,MONITOR_DEFAULTTONULL) ) SetWindowPlacement(g_wnd,&g_placement);
  else GetWindowPlacement(g_wnd,&g_placement);
  ShowWindow(g_wnd,g_placement.length && g_placement.showCmd==SW_SHOWMAXIMIZED ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
  if( g_settings.fullScreen ) SetFullScreen(true);
  return true;

}

} // namespace

//-----------------------------------------------------------------------------

int WINAPI wWinMain(_In_ HINSTANCE inst,_In_opt_ HINSTANCE,_In_ PWSTR,_In_ int) {

  // One instance only: two would overwrite each other's high scores
  HANDLE single = CreateMutexW(nullptr,FALSE,L"BlockOut3.SingleInstance");
  if( single && GetLastError()==ERROR_ALREADY_EXISTS ) {
    if( HWND other = FindWindowW(L"BlockOut3",nullptr) ) {
      if( IsIconic(other) ) ShowWindow(other,SW_RESTORE);
      SetForegroundWindow(other);
    }
    return 0;
  }

  LoadSettings(g_settings,g_placement);
  LoadHighScores(g_scores);

  if( !CreateMainWindow(inst) ) {
    MessageBoxW(nullptr,L"The window could not be created.",L"Block Out III",MB_OK|MB_ICONERROR);
    return 1;
  }

  LARGE_INTEGER freq, start, now;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&start);
  auto seconds = [&] {
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - start.QuadPart) / (double)freq.QuadPart;
  };

  if( !g_renderer.Init(g_wnd) ) {
    MessageBoxW(g_wnd,L"Direct3D 11 could not be initialised.",L"Block Out III",MB_OK|MB_ICONERROR);
    return 1;
  }

  g_audio.Init(&g_settings.playSound);
  // On the heap: the game logic and the menu are large (bot search buffers)
  auto gamePtr = std::make_unique<GameLogic>();
  auto menuPtr = std::make_unique<Menu>(g_settings,g_scores);
  GameLogic &game = *gamePtr;
  Menu &menu = *menuPtr;
  game.SetSoundSink(&g_audio);
  menu.SetSoundSink(&g_audio);
  bool inMenu = true;

  // High scores are saved at once; a failure (disk full, no rights) is
  // reported once per session
  bool saveWarned = false;
  auto saveScores = [&] {
    if( !SaveHighScores(g_scores) && !saveWarned ) {
      saveWarned = true;
      MessageBoxW(g_wnd,L"The high scores could not be saved to %LOCALAPPDATA%\\Block Out III.",
                  L"Block Out III",MB_OK|MB_ICONWARNING);
    }
  };
  auto save = [&] {
    if( !g_settings.fullScreen ) GetWindowPlacement(g_wnd,&g_placement);
    SaveSettings(g_settings,g_placement);
    saveScores();
  };

  // Times given to the game are seconds since the start of the current game:
  // a float of seconds since the program start would lose precision after hours
  double epoch = 0.0;
  auto startGame = [&](MenuAction a,float &t) {
    epoch = seconds() - 1.0;
    t = 1.0f;
    GameConfig cfg = g_settings.MakeGameConfig(g_scores.Best(g_settings.GetSetupId()));
    uint32 date = (uint32)time(nullptr);
    if( a==MenuAction::Demo )          game.StartDemo(cfg,t,date,date);
    else if( a==MenuAction::Practice ) game.StartPractice(cfg,t,date,date);
    else                               game.StartGame(cfg,t,date,date);
    memset(g_keys,0,sizeof(g_keys));
    inMenu = false;
  };

#ifdef BO3_PERF
  // Frame timing statistics, written to %TEMP%o3perf.txt at exit
  struct PerfFrame { double start, logic, render; };
  std::vector<PerfFrame> perf;
  perf.reserve(100000);
#endif

  bool quit = false;
  while( !quit ) {

    MSG msg;
    while( PeekMessageW(&msg,nullptr,0,0,PM_REMOVE) ) {
      if( msg.message==WM_QUIT ) quit = true;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    if( quit ) break;

    if( IsIconic(g_wnd) ) { WaitMessage(); continue; }

    // Sleep until the swap chain accepts a frame (VSync) or input arrives
    HANDLE frame = g_renderer.FrameWaitable();
    if( MsgWaitForMultipleObjectsEx(1,&frame,1000,QS_ALLINPUT,MWMO_INPUTAVAILABLE)!=WAIT_OBJECT_0 )
      continue;

#ifdef BO3_PERF
    double perfStart = seconds();
#endif
    float t = (float)(seconds() - epoch);
    if( inMenu ) {
      MenuAction a = menu.Process(g_keys,t);
      memset(g_keys,0,sizeof(g_keys));   // keys the menu did not use are dropped
      switch(a) {
        case MenuAction::StartGame:
        case MenuAction::Demo:
        case MenuAction::Practice:
          save();
          startGame(a,t);
          break;
        case MenuAction::Quit:
          PostMessageW(g_wnd,WM_CLOSE,0,0);
          break;
        case MenuAction::ToggleFullScreen:
          SetFullScreen(!g_settings.fullScreen);
          save();
          break;
        case MenuAction::SettingsChanged:
          save();
          break;
        case MenuAction::None:
          break;
      }
    } else {
      // [O]: sound off/on, as in the DOS Blockout (unless O rotates the block)
      const Settings &s = g_settings;
      bool oRotates = 'O'==s.keyRx1 || 'O'==s.keyRy1 || 'O'==s.keyRz1 || 'O'==s.keyRx2 || 'O'==s.keyRy2 || 'O'==s.keyRz2;
      if( !oRotates && (g_keys['o'] || g_keys['O']) ) {
        g_settings.playSound = !g_settings.playSound;
        g_keys['o'] = g_keys['O'] = 0;
        save();
      }
      switch( game.Process(g_keys,t) ) {
        case 1: {   // end of a game: high score table
          int rank = g_scores.Insert(game.GetScore());
          if( rank<10 ) {
            saveScores();
            g_audio.Play(g_settings.soundType==SOUND_BLOCKOUT2 ? Sound::WellDone : Sound::WellDone2);
          }
          menu.ShowHallOfFame(rank);
          memset(g_keys,0,sizeof(g_keys));
          inMenu = true;
          break;
        }
        case 2:     // end of a demo or a practice
          menu.ShowMain();
          memset(g_keys,0,sizeof(g_keys));
          inMenu = true;
          break;
      }
    }

#ifdef BO3_PERF
    double perfLogic = seconds();
#endif
    bool rendered = g_renderer.Render(&game,inMenu ? &menu : nullptr,g_settings);
#ifdef BO3_PERF
    perf.push_back(PerfFrame{ perfStart, perfLogic-perfStart, seconds()-perfLogic });
#endif
    if( !rendered ) {
      MessageBoxW(g_wnd,L"The graphics device was lost and could not be restored.",L"Block Out III",MB_OK|MB_ICONERROR);
      break;
    }

  }

#ifdef BO3_PERF
  if( perf.size() > 120 ) {
    perf.erase(perf.begin(),perf.begin()+60);   // skip start up
    std::vector<double> iv, lg, rd;
    for(size_t i=1;i<perf.size();i++) iv.push_back((perf[i].start-perf[i-1].start)*1000.0);
    for(const PerfFrame &f : perf) { lg.push_back(f.logic*1000.0); rd.push_back(f.render*1000.0); }
    auto stat = [](std::vector<double> v,const char *name,FILE *f) {
      std::sort(v.begin(),v.end());
      double sum = 0; for(double x : v) sum += x;
      fprintf(f,"%-8s avg %7.3f  p50 %7.3f  p99 %7.3f  max %7.3f ms\n",name,sum/v.size(),
              v[v.size()/2],v[v.size()*99/100],v.back());
    };
    std::vector<double> sorted = iv;
    std::sort(sorted.begin(),sorted.end());
    double median = sorted[sorted.size()/2];
    size_t missed = 0;
    for(double x : iv) if( x > median*1.5 ) missed++;
    wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH,path);
    wcscat_s(path,L"bo3perf.txt");
    FILE *f = nullptr;
    if( !_wfopen_s(&f,path,L"w") && f ) {
      fprintf(f,"frames %zu  missed vsync %zu\n",perf.size(),missed);
      stat(iv,"interval",f);
      stat(lg,"logic",f);
      stat(rd,"render",f);
      fclose(f);
    }
  }
#endif

  // Release the GPU objects now, not in a static destructor at process exit
  g_renderer.Shutdown();
  g_audio.Shutdown();
  if( single ) CloseHandle(single);
  SaveSettings(g_settings,g_placement);
  return 0;

}
