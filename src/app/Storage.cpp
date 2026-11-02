/*
  File:        Storage.cpp
  Description: Settings and high score files in %LOCALAPPDATA%\Block Out III
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
#include "Storage.h"
#include <shlobj.h>
#include <string>

namespace bo3 {

//-----------------------------------------------------------------------------

static std::wstring LocalAppData(const wchar_t *sub) {

  PWSTR base = nullptr;
  std::wstring dir;
  if( SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&base)) )
    dir = std::wstring(base) + L"\\" + sub;
  CoTaskMemFree(base);
  return dir;

}

// Our data directory, created on demand (empty string when unavailable)
static std::wstring DataDir() {

  static std::wstring dir = [] {
    std::wstring d = LocalAppData(L"Block Out III");
    if( d.empty() || (!CreateDirectoryW(d.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) )
      return std::wstring();
    return d;
  }();
  return dir;

}

static std::wstring DataFile(const wchar_t *name) {
  std::wstring d = DataDir();
  return d.empty() ? d : d + L"\\" + name;
}

// BlockOut II data directory
static std::wstring OldFile(const wchar_t *name) {
  std::wstring d = LocalAppData(L"Blockout");
  return d.empty() ? d : d + L"\\" + name;
}

//-----------------------------------------------------------------------------

static bool ReadAll(const std::wstring &path,std::vector<byte> &data) {

  data.clear();
  if( path.empty() ) return false;
  HANDLE h = CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
  if( h==INVALID_HANDLE_VALUE ) return false;
  LARGE_INTEGER size;
  bool ok = GetFileSizeEx(h,&size) && size.QuadPart < (1<<20);
  if( ok ) {
    data.resize((size_t)size.QuadPart);
    DWORD got = 0;
    ok = ReadFile(h,data.data(),(DWORD)data.size(),&got,nullptr) && got==data.size();
  }
  CloseHandle(h);
  return ok;

}

// Write to a temporary file, then replace: the old file survives a crash or
// a full disk
static bool WriteAtomic(const std::wstring &path,const std::vector<byte> &data) {

  if( path.empty() ) return false;
  std::wstring tmp = path + L".tmp";
  HANDLE h = CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);
  if( h==INVALID_HANDLE_VALUE ) return false;
  DWORD put = 0;
  bool ok = WriteFile(h,data.data(),(DWORD)data.size(),&put,nullptr) && put==data.size() &&
            FlushFileBuffers(h);
  CloseHandle(h);
  ok = ok && MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
  if( !ok ) DeleteFileW(tmp.c_str());
  return ok;

}

//-----------------------------------------------------------------------------

void LoadHighScores(HighScores &hs) {

  std::vector<byte> data;
  if( ReadAll(DataFile(L"hscore.dat"),data) ) {
    hs.Parse(data);
  } else if( ReadAll(OldFile(L"hscore.dat"),data) && hs.Parse(data) ) {
    // First start: take over the BlockOut II high scores (its file is left as is)
    SaveHighScores(hs);
  }

}

bool SaveHighScores(const HighScores &hs) {
  return WriteAtomic(DataFile(L"hscore.dat"),hs.Serialize());
}

//-----------------------------------------------------------------------------
// settings.ini

struct IniField { const wchar_t *section; const wchar_t *key; int Settings::*value; };
struct IniFlag  { const wchar_t *section; const wchar_t *key; bool Settings::*value; };
struct IniFx    { const wchar_t *key; bool FxSettings::*value; };

static const IniField intFields[] = {
  { L"Game", L"PitWidth", &Settings::pitWidth },
  { L"Game", L"PitHeight", &Settings::pitHeight },
  { L"Game", L"PitDepth", &Settings::pitDepth },
  { L"Game", L"BlockSet", &Settings::blockSet },
  { L"Game", L"AnimationSpeed", &Settings::animationSpeed },
  { L"Game", L"StartLevel", &Settings::startLevel },
  { L"Keys", L"RotateX1", &Settings::keyRx1 },
  { L"Keys", L"RotateY1", &Settings::keyRy1 },
  { L"Keys", L"RotateZ1", &Settings::keyRz1 },
  { L"Keys", L"RotateX2", &Settings::keyRx2 },
  { L"Keys", L"RotateY2", &Settings::keyRy2 },
  { L"Keys", L"RotateZ2", &Settings::keyRz2 },
  { L"Graphics", L"Transparency", &Settings::transparentFace },
  { L"Graphics", L"LineWidth", &Settings::lineWidth },
  { L"Graphics", L"EffectPreset", &Settings::fxPreset },
  { L"Sound", L"Type", &Settings::soundType },
};
static const IniFlag flagFields[] = {
  { L"Graphics", L"FullScreen", &Settings::fullScreen },
  { L"Sound", L"Enabled", &Settings::playSound },
};
static const IniFx fxFields[] = {
  { L"Scanlines", &FxSettings::scanlines }, { L"Vignette", &FxSettings::vignette },
  { L"Glow", &FxSettings::glow },           { L"Afterglow", &FxSettings::afterglow },
  { L"Curvature", &FxSettings::curvature }, { L"ApertureMask", &FxSettings::mask },
  { L"EgaPalette", &FxSettings::ega },      { L"LowRes320x200", &FxSettings::lowRes },
};

void LoadSettings(Settings &s,WINDOWPLACEMENT &placement) {

  placement = {};
  std::wstring ini = DataFile(L"settings.ini");

  if( ini.empty() || GetFileAttributesW(ini.c_str())==INVALID_FILE_ATTRIBUTES ) {
    // First start: take over the BlockOut II setup when there is one
    std::vector<byte> data;
    if( ReadAll(OldFile(L"setup.dat"),data) ) ImportBlockOut2Setup(data,s);
    return;
  }

  const wchar_t *f = ini.c_str();
  for(const IniField &i : intFields)
    s.*i.value = (int)GetPrivateProfileIntW(i.section,i.key,s.*i.value,f);
  for(const IniFlag &i : flagFields)
    s.*i.value = GetPrivateProfileIntW(i.section,i.key,s.*i.value,f)!=0;
  for(const IniFx &i : fxFields)
    s.fx.*i.value = GetPrivateProfileIntW(L"Effects",i.key,s.fx.*i.value,f)!=0;
  s.Sanitize();
  if( s.fxPreset!=FX_CUSTOM ) s.fx = FxFromPreset(s.fxPreset);

  // GetPrivateProfileInt() turns negative values (monitors left of the
  // primary one) into 0, so these are read as strings
  auto getSigned = [f](const wchar_t *key) {
    wchar_t buf[32];
    GetPrivateProfileStringW(L"Window",key,L"0",buf,32,f);
    return (LONG)_wtoi(buf);
  };
  RECT &r = placement.rcNormalPosition;
  r.left   = getSigned(L"Left");
  r.top    = getSigned(L"Top");
  r.right  = getSigned(L"Right");
  r.bottom = getSigned(L"Bottom");
  if( r.right-r.left>=160 && r.bottom-r.top>=120 ) {
    placement.length = sizeof(placement);
    placement.showCmd = GetPrivateProfileIntW(L"Window",L"Maximized",0,f) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
  }

}

static void PutInt(const wchar_t *section,const wchar_t *key,int v,const wchar_t *file) {
  WritePrivateProfileStringW(section,key,std::to_wstring(v).c_str(),file);
}

void SaveSettings(const Settings &s,const WINDOWPLACEMENT &placement) {

  std::wstring ini = DataFile(L"settings.ini");
  if( ini.empty() ) return;
  const wchar_t *f = ini.c_str();

  for(const IniField &i : intFields) PutInt(i.section,i.key,s.*i.value,f);
  for(const IniFlag &i : flagFields) PutInt(i.section,i.key,s.*i.value,f);
  for(const IniFx &i : fxFields) PutInt(L"Effects",i.key,s.fx.*i.value,f);

  if( placement.length ) {
    const RECT &r = placement.rcNormalPosition;
    PutInt(L"Window",L"Left",r.left,f);
    PutInt(L"Window",L"Top",r.top,f);
    PutInt(L"Window",L"Right",r.right,f);
    PutInt(L"Window",L"Bottom",r.bottom,f);
    PutInt(L"Window",L"Maximized",placement.showCmd==SW_SHOWMAXIMIZED,f);
  }

}

} // namespace bo3
