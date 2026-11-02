/*
  File:        Audio.h
  Description: Sound playback with XAudio2
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
#pragma once

#include <windows.h>
#include <xaudio2.h>
#include <wrl/client.h>
#include <vector>
#include "Synth.h"

namespace bo3 {

class Audio : public SoundSink {

  public:

    ~Audio() { Shutdown(); }

    // Renders all sounds and opens the audio device. Without a device the
    // game runs silently. enabled: the sound setting, checked at each Play.
    void Init(const bool *enabled);
    void Shutdown();

    void Play(Sound s) override;

  private:

    static const int NB_VOICES = 8;

    Microsoft::WRL::ComPtr<IXAudio2> engine;
    IXAudio2MasteringVoice *master = nullptr;
    IXAudio2SourceVoice    *voices[NB_VOICES] = {};
    int                     nextVoice = 0;
    std::vector<int16_t>    pcm[SOUND_COUNT];
    const bool             *enabled = nullptr;

};

} // namespace bo3
