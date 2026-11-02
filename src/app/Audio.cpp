/*
  File:        Audio.cpp
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
#include "Audio.h"

#pragma comment(lib,"xaudio2.lib")

namespace bo3 {

void Audio::Init(const bool *on) {

  enabled = on;
  for(int i=0;i<SOUND_COUNT;i++) pcm[i] = Synthesize((Sound)i);

  if( FAILED(XAudio2Create(&engine,0,XAUDIO2_DEFAULT_PROCESSOR)) ||
      FAILED(engine->CreateMasteringVoice(&master)) ) {
    Shutdown();
    return;
  }

  WAVEFORMATEX fmt = {};
  fmt.wFormatTag = WAVE_FORMAT_PCM;
  fmt.nChannels = 1;
  fmt.nSamplesPerSec = SYNTH_RATE;
  fmt.wBitsPerSample = 16;
  fmt.nBlockAlign = 2;
  fmt.nAvgBytesPerSec = SYNTH_RATE * 2;
  for(IXAudio2SourceVoice *&v : voices)
    if( FAILED(engine->CreateSourceVoice(&v,&fmt)) ) v = nullptr;

}

void Audio::Shutdown() {

  for(IXAudio2SourceVoice *&v : voices)
    if( v ) { v->DestroyVoice(); v = nullptr; }
  if( master ) { master->DestroyVoice(); master = nullptr; }
  engine.Reset();

}

void Audio::Play(Sound s) {

  if( !engine || !enabled || !*enabled ) return;
  const std::vector<int16_t> &data = pcm[(int)s];
  if( data.empty() ) return;

  // A free voice, otherwise the oldest one is cut
  IXAudio2SourceVoice *voice = nullptr;
  for(IXAudio2SourceVoice *v : voices) {
    XAUDIO2_VOICE_STATE st;
    if( v ) { v->GetState(&st,XAUDIO2_VOICE_NOSAMPLESPLAYED); if( st.BuffersQueued==0 ) { voice = v; break; } }
  }
  if( !voice ) {
    voice = voices[nextVoice];
    nextVoice = (nextVoice + 1) % NB_VOICES;
    if( !voice ) return;
    voice->Stop();
    voice->FlushSourceBuffers();
  }

  XAUDIO2_BUFFER buf = {};
  buf.AudioBytes = (UINT32)(data.size() * sizeof(int16_t));
  buf.pAudioData = (const BYTE *)data.data();
  buf.Flags = XAUDIO2_END_OF_STREAM;
  if( SUCCEEDED(voice->SubmitSourceBuffer(&buf)) ) voice->Start();

}

} // namespace bo3
